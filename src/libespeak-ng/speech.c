/*
 * Copyright (C) 2005 to 2013 by Jonathan Duddington
 * email: jonsd@users.sourceforge.net
 * Copyright (C) 2013-2017 Reece H. Dunn
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see: <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <locale.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>

#if defined(_WIN32) || defined(_WIN64)
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#include <winreg.h>
#endif

#include <espeak-ng/espeak_ng.h>
#include <espeak-ng/speak_lib.h>
#include <espeak-ng/encoding.h>

#include "speech.h"
#include "common.h"               // for GetFileLength
#include "dictionary.h"           // for GetTranslatedPhonemeString, strncpy0
#include "espeak_command.h"       // for delete_espeak_command, SetParameter
#include "event.h"                // for event_declare, event_clear_all, eve...
#include "fifo.h"                 // for fifo_add_command, fifo_add_commands
#include "langopts.h"             // for LoadConfig
#include "readclause.h"           // for PARAM_STACK, param_stack
#include "synthdata.h"            // for FreePhData, LoadPhData
#include "synthesize.h"           // for SpeakNextClause, Generate, Synthesi...
#include "translate.h"            // for p_decoder, InitText, translator
#include "voice.h"                // for FreeVoiceList, VoiceReset, current_...
#include "wavegen.h"              // for stub prototypes

// Global stubs - these were originally defined in wavegen.c (removed)
int samplerate = 22050;
intptr_t wcmdq[N_WCMDQ][4];
int wcmdq_head = 0;
int wcmdq_tail = 0;
// Note: wavefile_data is defined in synthdata.c, formant_rate in voices.c
int echo_head = 0;
int echo_tail = 0;
int echo_amp = 0;
short echo_buf[N_ECHO_BUF];
unsigned char *out_ptr = NULL;
unsigned char *out_end = NULL;
SPEED_FACTORS speed;

// Global variables originally in synthesize.c (original version, removed)
int embedded_value[N_EMBEDDED_VALUES];
const int embedded_default[N_EMBEDDED_VALUES] = {
    0,     // 0
    espeakRATE_NORMAL, // rate
    100,   // volume
    50,    // pitch
    50,    // range
    0,     // punctuation
    0,     // capitals
    0,     // wordgap
    0,     // options
    0,     // intonation
    0,
    0,
    0,     // emphasis
    0,     // line length
    0,     // voice type
};

// Stubs for mbrola globals (originally in mbrola.c)
char mbrola_name[20] = {0};
int mbrola_delay = 0;

// Stub function implementations (originally in wavegen.c & synthesize.c)
void WavegenInit(int rate, int wavemult) { (void)rate; (void)wavemult; }
void WavegenFill(void) {}
void WavegenFini(void) {}
int WcmdqUsed(void) { return 0; }
void WcmdqStop(void) {}
void WcmdqInc(void) {}
void GetAmplitude(void) {}
void InitBreath(void) {}
void WavegenSetVoice(voice_t *v) { (void)v; }
espeak_ng_STATUS DoVoiceChange(voice_t *v) { (void)v; return ENS_OK; }
void DoSonicSpeed(int value) { (void)value; }

void Write4Bytes(FILE *f, int value) {
    int ix;
    for (ix = 0; ix < 4; ix++) {
        fputc(value & 0xff, f);
        value = value >> 8;
    }
}

int FormantTransition2(frameref_t *seq, int *n_frames, 
    unsigned int data1, unsigned int data2,
    PHONEME_TAB *other_ph, int which) {
    (void)seq; (void)n_frames; (void)data1; (void)data2; (void)other_ph; (void)which;
    return 0;
}

void SetEmbedded(int control, int value) {
    int sign = 0;
    int command = control & 0x1f;

    if (command >= N_EMBEDDED_VALUES)
        return;

    if (control & 0x20)
        sign = -1;
    else if (control & 0x40)
        sign = 1;

    if (sign == 0)
        embedded_value[command] = value;
    else if (sign == 1)
        embedded_value[command] = embedded_default[command] + value;
    else
        embedded_value[command] = embedded_default[command] - value;
}



static unsigned char *outbuf = NULL;
static int outbuf_size = 0;
static unsigned char *out_start;

espeak_EVENT *event_list = NULL;
static int event_list_ix = 0;
static int n_event_list;
static long count_samples;

static unsigned int my_unique_identifier = 0;
static void *my_user_data = NULL;
static espeak_ng_OUTPUT_MODE my_mode = ENOUTPUT_MODE_SYNCHRONOUS;
static int out_samplerate = 0;
static int voice_samplerate = 22050;
static const int min_buffer_length = 60; // minimum buffer length in ms
static espeak_ng_STATUS err = ENS_OK;

static t_espeak_callback *synth_callback = NULL;

char path_home[N_PATH_HOME]; // this is the espeak-ng-data directory
extern int saved_parameters[N_SPEECH_PARAM]; // Parameters saved on synthesis start

void cancel_audio(void) {}

static int check_data_path(const char *path, int allow_directory)
{
	if (!path) return 0;

	snprintf(path_home, sizeof(path_home), "%s/espeak-ng-data", path);
	if (GetFileLength(path_home) == -EISDIR)
		return 1;

	if (!allow_directory)
		return 0;

	snprintf(path_home, sizeof(path_home), "%s", path);
	return GetFileLength(path_home) == -EISDIR;
}

#pragma GCC visibility push(default)

ESPEAK_NG_API espeak_ng_STATUS espeak_ng_InitializeOutput(espeak_ng_OUTPUT_MODE output_mode, int buffer_length, const char *device)
{
	(void)device; 
	my_mode = output_mode;
	out_samplerate = 0;
	return ENS_OK;
}


ESPEAK_NG_API void espeak_ng_InitializePath(const char *path)
{
	if (check_data_path(path, 1))
		return;

#if PLATFORM_WINDOWS
	HKEY RegKey;
	unsigned long size;
	unsigned long var_type;
	unsigned char buf[sizeof(path_home)-13];

	if (check_data_path(getenv("ESPEAK_DATA_PATH"), 1))
		return;

	buf[0] = 0;
	RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\eSpeak NG", 0, KEY_READ, &RegKey);
	if (RegKey == NULL)
		RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\WOW6432Node\\eSpeak NG", 0, KEY_READ, &RegKey);
	size = sizeof(buf);
	var_type = REG_SZ;
	RegQueryValueExA(RegKey, "Path", 0, &var_type, buf, &size);

	if (check_data_path(buf, 1))
		return;
#elif !defined(PLATFORM_DOS)
	if (check_data_path(getenv("ESPEAK_DATA_PATH"), 1))
		return;

	if (check_data_path(getenv("HOME"), 0))
		return;
#endif

	strcpy(path_home, PATH_ESPEAK_DATA);
}

const int param_defaults[N_SPEECH_PARAM] = {
	0,   // silence (internal use)
	espeakRATE_NORMAL, // rate wpm
	100, // volume
	50,  // pitch
	50,  // range
	0,   // punctuation
	0,   // capital letters
	0,   // wordgap
	0,   // options
	0,   // intonation
	100, // ssml break mul
	0,
	0,   // emphasis
	0,   // line length
	0,   // voice type
};


ESPEAK_NG_API espeak_ng_STATUS espeak_ng_Initialize(espeak_ng_ERROR_CONTEXT *context)
{
	int param;
	int srate = 22050; // default sample rate 22050 Hz

	// It seems that the wctype functions don't work until the locale has been set
	// to something other than the default "C".  Then, not only Latin1 but also the
	// other characters give the correct results with iswalpha() etc.
	if (setlocale(LC_CTYPE, "C.UTF-8") == NULL) {
		if (setlocale(LC_CTYPE, "UTF-8") == NULL) {
			if (setlocale(LC_CTYPE, "en_US.UTF-8") == NULL)
				setlocale(LC_CTYPE, "");
		}
	}

	espeak_ng_STATUS result = LoadPhData(&srate, context);
	if (result != ENS_OK)
		return result;

	// WavegenInit removed
	LoadConfig();

	espeak_VOICE *current_voice_selected = espeak_GetCurrentVoice();
	memset(current_voice_selected, 0, sizeof(espeak_VOICE));
	SetVoiceStack(NULL, "");
	SynthesizeInit();
	InitNamedata();

	VoiceReset(0);

	for (param = 0; param < N_SPEECH_PARAM; param++)
		param_stack[0].parameter[param] = saved_parameters[param] = param_defaults[param];

	SetParameter(espeakRATE, espeakRATE_NORMAL, 0);
	SetParameter(espeakVOLUME, 100, 0);
	SetParameter(espeakCAPITALS, option_capitals, 0);
	SetParameter(espeakPUNCTUATION, option_punctuation, 0);
	SetParameter(espeakWORDGAP, 0, 0);

	option_phonemes = 0;
	option_phoneme_events = 0;

	// Seed random generator
	espeak_srand(time(NULL));

	return ENS_OK;
}

ESPEAK_NG_API espeak_ng_STATUS espeak_ng_SetPhonemeEvents(int enable, int ipa) {
	option_phoneme_events = 0;
	if (enable) {
		option_phoneme_events |= espeakINITIALIZE_PHONEME_EVENTS;
		if (ipa) {
			option_phoneme_events |= espeakINITIALIZE_PHONEME_IPA;
		}
	}
	return ENS_OK;
}

ESPEAK_NG_API int espeak_ng_GetSampleRate(void)
{
	return samplerate;
}

#pragma GCC visibility pop

static espeak_ng_STATUS Synthesize(unsigned int unique_identifier, const void *text, int flags)
{
	// Fill the buffer with output sound
	// Removed logic

	option_ssml = flags & espeakSSML;
	option_phoneme_input = flags & espeakPHONEMES;
	option_endpause = flags & espeakENDPAUSE;

	count_samples = 0;

	espeak_ng_STATUS status;
	if (translator == NULL) {
		status = espeak_ng_SetVoiceByName(ESPEAKNG_DEFAULT_VOICE);
		if (status != ENS_OK)
			return status;
	}

	if (p_decoder == NULL)
		p_decoder = create_text_decoder();

	status = text_decoder_decode_string_multibyte(p_decoder, text, translator->encoding, flags);
	if (status != ENS_OK)
		return status;

	SpeakNextClause(0);

	while (1) {
		if (SpeakNextClause(1) == 0) {
			break;
		}
	}
	return ENS_OK;
}

void MarkerEvent(int type, unsigned int char_position, int value, int value2, unsigned char *out_ptr)
{
	// type: 1=word, 2=sentence, 3=named mark, 4=play audio, 5=end, 7=phoneme
	espeak_EVENT *ep;
	
	if ((event_list == NULL) || (event_list_ix >= (n_event_list-2)))
		return;

	ep = &event_list[event_list_ix++];
	ep->type = (espeak_EVENT_TYPE)type;
	ep->unique_identifier = my_unique_identifier;
	ep->user_data = my_user_data;
	ep->text_position = char_position & 0xffffff;
	ep->length = char_position >> 24;

	ep->audio_position = 0;
	ep->sample = 0;

	if ((type == espeakEVENT_MARK) || (type == espeakEVENT_PLAY))
		ep->id.name = &namedata[value];
	else if (type == espeakEVENT_PHONEME) {
		int *p;
		p = (int *)(ep->id.string);
		p[0] = value;
		p[1] = value2;
	} else
		ep->id.number = value;
}

espeak_ng_STATUS sync_espeak_Synth(unsigned int unique_identifier, const void *text,
                                   unsigned int position, espeak_POSITION_TYPE position_type,
                                   unsigned int end_position, unsigned int flags, void *user_data)
{
	InitText(flags);
	my_unique_identifier = unique_identifier;
	my_user_data = user_data;

	for (int i = 0; i < N_SPEECH_PARAM; i++)
		saved_parameters[i] = param_stack[0].parameter[i];

	switch (position_type)
	{
	case POS_CHARACTER:
		skip_characters = position;
		break;
	case POS_WORD:
		skip_words = position;
		break;
	case POS_SENTENCE:
		skip_sentences = position;
		break;

	}
	if (skip_characters || skip_words || skip_sentences)
		skipping_text = true;

	end_character_position = end_position;

	espeak_ng_STATUS aStatus = Synthesize(unique_identifier, text, flags);

	return aStatus;
}

espeak_ng_STATUS sync_espeak_Synth_Mark(unsigned int unique_identifier, const void *text,
                                        const char *index_mark, unsigned int end_position,
                                        unsigned int flags, void *user_data)
{
	InitText(flags);

	my_unique_identifier = unique_identifier;
	my_user_data = user_data;

	if (index_mark != NULL) {
		strncpy0(skip_marker, index_mark, sizeof(skip_marker));
		skipping_text = true;
	}

	end_character_position = end_position;

	return Synthesize(unique_identifier, text, flags | espeakSSML);
}

espeak_ng_STATUS sync_espeak_Key(const char *key)
{
	// symbolic name, symbolicname_character  - is there a system resource of symbolic names per language?
	int letter;
	int ix;

	ix = utf8_in(&letter, key);
	if (key[ix] == 0) // a single character
		return sync_espeak_Char(letter);

	my_unique_identifier = 0;
	my_user_data = NULL;
	return Synthesize(0, key, 0); // speak key as a text string
}

espeak_ng_STATUS sync_espeak_Char(wchar_t character)
{
	// is there a system resource of character names per language?
	char buf[80];
	my_unique_identifier = 0;
	my_user_data = NULL;

	sprintf(buf, "<say-as interpret-as=\"tts:char\">&#%d;</say-as>", character);
	return Synthesize(0, buf, espeakSSML);
}

void sync_espeak_SetPunctuationList(const wchar_t *punctlist)
{
	// Set the list of punctuation which are spoken for "some".
	my_unique_identifier = 0;
	my_user_data = NULL;

	option_punctlist[0] = 0;
	if (punctlist != NULL) {
		wcsncpy(option_punctlist, punctlist, N_PUNCTLIST);
		option_punctlist[N_PUNCTLIST-1] = 0;
	}
}

#pragma GCC visibility push(default)

ESPEAK_API void espeak_SetSynthCallback(t_espeak_callback *SynthCallback)
{
	synth_callback = SynthCallback;
}

ESPEAK_NG_API espeak_ng_STATUS
espeak_ng_Synthesize(const void *text, size_t size,
                     unsigned int position,
                     espeak_POSITION_TYPE position_type,
                     unsigned int end_position, unsigned int flags,
                     unsigned int *unique_identifier, void *user_data)
{
	(void)size; // unused in non-async modes

	unsigned int temp_identifier;

	if (unique_identifier == NULL)
		unique_identifier = &temp_identifier;
	*unique_identifier = 0;

	return sync_espeak_Synth(0, text, position, position_type, end_position, flags, user_data);
}

ESPEAK_NG_API espeak_ng_STATUS
espeak_ng_SynthesizeMark(const void *text,
                         size_t size,
                         const char *index_mark,
                         unsigned int end_position,
                         unsigned int flags,
                         unsigned int *unique_identifier,
                         void *user_data)
{
	(void)size; // unused in non-async modes

	unsigned int temp_identifier;

	if (unique_identifier == NULL)
		unique_identifier = &temp_identifier;
	*unique_identifier = 0;

	return sync_espeak_Synth_Mark(0, text, index_mark, end_position, flags, user_data);
}

ESPEAK_NG_API espeak_ng_STATUS espeak_ng_SpeakKeyName(const char *key_name)
{
	return sync_espeak_Key(key_name);
}

ESPEAK_NG_API espeak_ng_STATUS espeak_ng_SpeakCharacter(wchar_t character)
{
	return sync_espeak_Char(character);
}

ESPEAK_API int espeak_GetParameter(espeak_PARAMETER parameter, int current)
{
	// current: 0=default value, 1=current value
	if (current)
		return param_stack[0].parameter[parameter];
	return param_defaults[parameter];
}

ESPEAK_NG_API espeak_ng_STATUS espeak_ng_SetParameter(espeak_PARAMETER parameter, int value, int relative)
{
	return SetParameter(parameter, value, relative);
}

ESPEAK_NG_API espeak_ng_STATUS espeak_ng_SetPunctuationList(const wchar_t *punctlist)
{
	sync_espeak_SetPunctuationList(punctlist);
	return ENS_OK;
}

ESPEAK_API void espeak_SetPhonemeTrace(int phonememode, FILE *stream)
{
	/* phonememode:  Controls the output of phoneme symbols for the text
	      bits 0-2:
	         value=0  No phoneme output (default)
	         value=1  Output the translated phoneme symbols for the text
	         value=2  as (1), but produces IPA phoneme names rather than ascii
	      bit 3:   output a trace of how the translation was done (showing the matching rules and list entries)
	      bit 4:   produce pho data for mbrola
	      bit 7:   use (bits 8-23) as a tie within multi-letter phonemes names
	      bits 8-23:  separator character, between phoneme names

	   stream   output stream for the phoneme symbols (and trace).  If stream=NULL then it uses stdout.
	*/

	option_phonemes = phonememode;
	f_trans = stream;
	if (stream == NULL)
		f_trans = stderr;
}

ESPEAK_API const char* espeak_TextToPhonemesWithTerminator(const void** textptr, int textmode, int phonememode, int* terminator)
{
	/* phoneme_mode
	    bit 1:   0=eSpeak's ascii phoneme names, 1= International Phonetic Alphabet (as UTF-8 characters).
	    bit 7:   use (bits 8-23) as a tie within multi-letter phonemes names
	    bits 8-23:  separator character, between phoneme names
	 */

	if (p_decoder == NULL)
		p_decoder = create_text_decoder();

	if (text_decoder_decode_string_multibyte(p_decoder, *textptr, translator->encoding, textmode) != ENS_OK)
		return NULL;

	TranslateClauseWithTerminator(translator, NULL, NULL, terminator);
	*textptr = text_decoder_get_buffer(p_decoder);

	return GetTranslatedPhonemeString(phonememode);
}

ESPEAK_API const char *espeak_TextToPhonemes(const void **textptr, int textmode, int phonememode)
{
	return espeak_TextToPhonemesWithTerminator(textptr, textmode, phonememode, NULL);
}

ESPEAK_NG_API espeak_ng_STATUS espeak_ng_Cancel(void)
{
	embedded_value[EMBED_T] = 0; // reset echo for pronunciation announcements

	for (int i = 0; i < N_SPEECH_PARAM; i++)
		SetParameter(i, saved_parameters[i], 0);

	return ENS_OK;
}

ESPEAK_API int espeak_IsPlaying(void)
{
	return 0;
}

ESPEAK_NG_API espeak_ng_STATUS espeak_ng_Synchronize(void)
{
	espeak_ng_STATUS berr = err;
	err = ENS_OK;
	return berr;
}

ESPEAK_NG_API espeak_ng_STATUS espeak_ng_Terminate(void)
{
	free(event_list);
	event_list = NULL;

	free(outbuf);
	outbuf = NULL;

	FreePhData();
	FreeVoiceList();

	DeleteTranslator(translator);
	translator = NULL;

	if (p_decoder != NULL) {
		destroy_text_decoder(p_decoder);
		p_decoder = NULL;
	}

	return ENS_OK;
}

static const char version_string[] = PACKAGE_VERSION;
ESPEAK_API const char *espeak_Info(const char **ptr)
{
	if (ptr != NULL)
		*ptr = path_home;
	return version_string;
}

#pragma GCC visibility pop
