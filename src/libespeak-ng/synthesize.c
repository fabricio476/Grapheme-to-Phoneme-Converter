/*
 * Copyright (C) 2005 to 2014 by Jonathan Duddington
 * email: jonsd@users.sourceforge.net
 * Copyright (C) 2015-2017 Reece H. Dunn
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

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <espeak-ng/espeak_ng.h>
#include <espeak-ng/speak_lib.h>
#include <espeak-ng/encoding.h>

#include "synthesize.h"
#include "dictionary.h"           // for WritePhMnemonic, GetTranslatedPhone...
#include "intonation.h"           // for CalcPitches
#include "phoneme.h"              // for PHONEME_TAB, phVOWEL, phLIQUID, phN...
#include "setlengths.h"           // for CalcLengths
#include "synthdata.h"            // for InterpretPhoneme, GetEnvelope, Inte...
#include "translate.h"            // for translator, LANGUAGE_OPTIONS, Trans...
#include "voice.h"                // for voice_t, voice, LoadVoiceVariant
#include "speech.h"               // for MAKE_MEM_UNDEFINED



// list of phonemes in a clause
int n_phoneme_list = 0;
PHONEME_LIST phoneme_list[N_PHONEME_LIST+1];

static voice_t *new_voice = NULL;

static int (*phoneme_callback)(const char *) = NULL;

#define RMS_GLOTTAL1 35   // vowel before glottal stop
#define RMS_START 28  // 28
#define VOWEL_FRONT_LENGTH  50

const char *WordToString(char buf[5], unsigned int word)
{
	// Convert a phoneme mnemonic word into a string
	int ix;

	for (ix = 0; ix < 4; ix++)
		buf[ix] = word >> (ix*8);
	buf[4] = 0;
	return buf;
}

void SynthesizeInit(void)
{
	// Stub
}

int SpeakNextClause(int control)
{
	// Speak text from memory (text_in)
	// control 0: start
	//    text_in is set

	// The other calls have text_in = NULL
	// control 1: speak next text
	//         2: stop

	int clause_tone;
	char *voice_change;

	if (control == 2) {
		// stop speaking
		n_phoneme_list = 0;
		return 0;
	}

	if (text_decoder_eof(p_decoder)) {
		skipping_text = false;
		return 0;
	}

	SelectPhonemeTable(voice->phoneme_tab_ix);

	// read the next clause from the input text file, translate it
	TranslateClause(translator, &clause_tone, &voice_change);

	CalcPitches(translator, clause_tone);
	CalcLengths(translator);

	if ((option_phonemes & 0xf) || (phoneme_callback != NULL)) {
		const char *phon_out;
		phon_out = GetTranslatedPhonemeString(option_phonemes);
		if (option_phonemes & 0xf)
			fprintf(f_trans, "%s\n", phon_out);
		if (phoneme_callback != NULL)
			phoneme_callback(phon_out);
	}

	if (skipping_text) {
		n_phoneme_list = 0;
		return 1;
	}

	// Audio generation removed

	if (voice_change != NULL) {
		// voice change at the end of the clause (i.e. clause was terminated by a voice change)
		new_voice = LoadVoiceVariant(voice_change, 0); 
	}

	if (new_voice) {
		// DoVoiceChange removed
		new_voice = NULL;
	}

	return 1;
}

#pragma GCC visibility push(default)
ESPEAK_API void espeak_SetPhonemeCallback(int (*PhonemeCallback)(const char *))
{
	phoneme_callback = PhonemeCallback;
}
#pragma GCC visibility pop
