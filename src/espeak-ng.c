/*
 * espeak-ng G2P command-line tool
 * Stripped down from the original espeak-ng for Grapheme-to-Phoneme only.
 *
 * Copyright (C) 2006 to 2013 by Jonathan Duddington
 * Copyright (C) 2015-2016 Reece H. Dunn
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

#include "config.h"

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include <espeak-ng/espeak_ng.h>
#include <espeak-ng/speak_lib.h>

#ifndef PROGRAM_NAME
#define PROGRAM_NAME "espeak-ng"
#endif

extern ESPEAK_NG_API void strncpy0(char *to, const char *from, int size);
extern ESPEAK_NG_API int utf8_in(int *c, const char *buf);
extern ESPEAK_NG_API int GetFileLength(const char *filename);

static const char *help_text =
    "\n" PROGRAM_NAME " [options] [\"<words>\"]\n\n"
    "G2P (Grapheme-to-Phoneme) converter based on espeak-ng\n\n"
    "-f <text file>   Text file to process\n"
    "--stdin    Read text input from stdin\n\n"
    "-v <voice name>\n"
    "\t   Use voice file of this name from espeak-ng-data/voices\n"
    "-x\t   Write phoneme mnemonics to stdout\n"
    "-X\t   Write phonemes mnemonics and translation trace to stdout\n"
    "--ipa      Write phonemes to stdout using International Phonetic Alphabet\n"
    "--path=\"<path>\"\n"
    "\t   Specifies the directory containing the espeak-ng-data directory\n"
    "--phonout=\"<filename>\"\n"
    "\t   Write phoneme output to this file\n"
    "--sep=<character>\n"
    "\t   Separate phonemes with <character>.\n"
    "--tie=<character>\n"
    "\t   Use a tie character within multi-letter phoneme names.\n"
    "--compile=<voice name>\n"
    "\t   Compile pronunciation rules and dictionary.\n"
    "--voices=<language>\n"
    "\t   List the available voices for the specified language.\n"
    "--version  Shows version number\n"
    "-h, --help Show this help.\n";

static void DisplayVoices(FILE *f_out, char *language)
{
	int ix;
	const char *p;
	int len;
	int count;
	int c;
	size_t j;
	const espeak_VOICE *v;
	const char *lang_name;
	char age_buf[12];
	char buf[80];
	const espeak_VOICE **voices;
	espeak_VOICE voice_select;

	static const char genders[4] = { '-', 'M', 'F', '-' };

	if ((language != NULL) && (language[0] != 0)) {
		voice_select.languages = language;
		voice_select.age = 0;
		voice_select.gender = 0;
		voice_select.name = NULL;
		voices = espeak_ListVoices(&voice_select);
	} else
		voices = espeak_ListVoices(NULL);

	fprintf(f_out, "Pty Language       Age/Gender VoiceName          File                 Other Languages\n");

	for (ix = 0; (v = voices[ix]) != NULL; ix++) {
		count = 0;
		p = v->languages;
		while (*p != 0) {
			len = strlen(p+1);
			lang_name = p+1;

			if (v->age == 0)
				strcpy(age_buf, " --");
			else
				sprintf(age_buf, "%3d", v->age);

			if (count == 0) {
				for (j = 0; j < sizeof(buf); j++) {
					if ((c = v->name[j]) == ' ')
						c = '_';
					if ((buf[j] = c) == 0)
						break;
				}
				fprintf(f_out, "%2d  %-15s%s/%c      %-18s %-20s ",
				        p[0], lang_name, age_buf, genders[v->gender], buf, v->identifier);
			} else
				fprintf(f_out, "(%s %d)", lang_name, p[0]);
			count++;
			p += len+2;
		}
		fputc('\n', f_out);
	}
}

static void PrintVersion()
{
	const char *version;
	const char *path_data;
	espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS, 0, NULL, espeakINITIALIZE_DONT_EXIT);
	version = espeak_Info(&path_data);
	printf("eSpeak NG G2P: %s  Data at: %s\n", version, path_data);
}

int main(int argc, char **argv)
{
	static const struct option long_options[] = {
		{ "help",    no_argument,       0, 'h' },
		{ "stdin",   no_argument,       0, 0x100 },
		{ "compile-debug", optional_argument, 0, 0x101 },
		{ "compile", optional_argument, 0, 0x102 },
		{ "voices",  optional_argument, 0, 0x104 },
		{ "path",    required_argument, 0, 0x107 },
		{ "phonout", required_argument, 0, 0x108 },
		{ "ipa",     optional_argument, 0, 0x10a },
		{ "version", no_argument,       0, 0x10b },
		{ "sep",     optional_argument, 0, 0x10c },
		{ "tie",     optional_argument, 0, 0x10d },
		{ "load",    no_argument,       0, 0x111 },
		{ 0, 0, 0, 0 }
	};

	FILE *f_text = NULL;
	char *p_text = NULL;
	FILE *f_phonemes_out = stdout;
	char *data_path = NULL;

	int option_index = 0;
	int c;
	int ix;
	char *optarg2;
	int value;
	int flag_stdin = 0;
	int flag_compile = 0;
	int flag_load = 0;
	int filesize = 0;
	int synth_flags = espeakCHARS_AUTO | espeakPHONEMES | espeakENDPAUSE;

	int speed = -1;
	int pitch = -1;
	int pitch_range = -1;
	int wordgap = -1;
	int phonemes_separator = 0;
	int phoneme_options = 0;
	int option_linelength = 0;

	espeak_VOICE voice_select;
	char filename[200];
	char voicename[40];

	voicename[0] = 0;
	filename[0] = 0;

	while (true) {
		c = getopt_long(argc, argv, "b:f:hk:l:p:P:s:v:g:xXz",
		                long_options, &option_index);

		if (c == -1)
			break;
		optarg2 = optarg;

		switch (c)
		{
		case 'b':
			if ((sscanf(optarg2, "%d", &value) == 1) && (value <= 4))
				synth_flags |= value;
			else
				synth_flags |= espeakCHARS_8BIT;
			break;
		case 'h':
			printf("\n");
			PrintVersion();
			printf("%s", help_text);
			return 0;
		case 'x':
			phoneme_options |= espeakPHONEMES_SHOW;
			break;
		case 'X':
			phoneme_options |= espeakPHONEMES_TRACE;
			break;
		case 'p':
			pitch = atoi(optarg2);
			break;
		case 'P':
			pitch_range = atoi(optarg2);
			break;
		case 'f':
			strncpy0(filename, optarg2, sizeof(filename));
			break;
		case 'l':
			option_linelength = atoi(optarg2);
			break;
		case 's':
			speed = atoi(optarg2);
			break;
		case 'g':
			wordgap = atoi(optarg2);
			break;
		case 'v':
			strncpy0(voicename, optarg2, sizeof(voicename));
			break;
		case 'z':
			synth_flags &= ~espeakENDPAUSE;
			break;
		case 0x100: // --stdin
			flag_stdin = 1;
			break;
		case 0x101: // --compile-debug
		case 0x102: // --compile
			if (optarg2 != NULL && *optarg2) {
				strncpy0(voicename, optarg2, sizeof(voicename));
				flag_compile = c;
				break;
			} else {
				fprintf(stderr, "Voice name not specified.\n");
				exit(EXIT_FAILURE);
			}
		case 0x104: // --voices
			espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS, 0, data_path, 0);
			DisplayVoices(stdout, optarg2);
			exit(0);
		case 0x107: // --path
			data_path = optarg2;
			break;
		case 0x108: // --phonout
			if ((f_phonemes_out = fopen(optarg2, "w")) == NULL)
				fprintf(stderr, "Can't write to: %s\n", optarg2);
			break;
		case 0x10a: // --ipa
			phoneme_options |= espeakPHONEMES_IPA;
			if (optarg2 != NULL) {
				switch (atoi(optarg2))
				{
				case 1:
					phonemes_separator = '_';
					break;
				case 2:
					phonemes_separator = 0x0361;
					phoneme_options |= espeakPHONEMES_TIE;
					break;
				case 3:
					phonemes_separator = 0x200d;
					phoneme_options |= espeakPHONEMES_TIE;
					break;
				}
			}
			break;
		case 0x10b: // --version
			PrintVersion();
			exit(0);
		case 0x10c: // --sep
			phoneme_options |= espeakPHONEMES_SHOW;
			if (optarg2 == 0)
				phonemes_separator = ' ';
			else
				utf8_in(&phonemes_separator, optarg2);
			if (phonemes_separator == 'z')
				phonemes_separator = 0x200c;
			break;
		case 0x10d: // --tie
			phoneme_options |= (espeakPHONEMES_SHOW | espeakPHONEMES_TIE);
			if (optarg2 == 0)
				phonemes_separator = 0x0361;
			else
				utf8_in(&phonemes_separator, optarg2);
			if (phonemes_separator == 'z')
				phonemes_separator = 0x200d;
			break;
		case 0x111: // --load
			flag_load = 1;
			break;
		default:
			exit(0);
		}
	}

	espeak_ng_InitializePath(data_path);
	espeak_ng_ERROR_CONTEXT context = NULL;
	espeak_ng_STATUS result = espeak_ng_Initialize(&context);
	if (result != ENS_OK) {
		espeak_ng_PrintStatusCodeMessage(result, stderr, context);
		espeak_ng_ClearErrorContext(&context);
		exit(1);
	}

	result = espeak_ng_InitializeOutput(ENOUTPUT_MODE_SYNCHRONOUS, 0, NULL);

	if (result != ENS_OK) {
		espeak_ng_PrintStatusCodeMessage(result, stderr, NULL);
		exit(EXIT_FAILURE);
	}

	if (voicename[0] == 0)
		strcpy(voicename, ESPEAKNG_DEFAULT_VOICE);

	if(flag_load)
		result = espeak_ng_SetVoiceByFile(voicename);
	else
		result = espeak_ng_SetVoiceByName(voicename);
	if (result != ENS_OK) {
		memset(&voice_select, 0, sizeof(voice_select));
		voice_select.languages = voicename;
		result = espeak_ng_SetVoiceByProperties(&voice_select);
		if (result != ENS_OK) {
			espeak_ng_PrintStatusCodeMessage(result, stderr, NULL);
			exit(EXIT_FAILURE);
		}
	}

	if (flag_compile) {
		espeak_ng_ERROR_CONTEXT context = NULL;
		espeak_ng_STATUS result = espeak_ng_CompileDictionary("", NULL, stderr, flag_compile & 0x1, &context);
		if (result != ENS_OK) {
			espeak_ng_PrintStatusCodeMessage(result, stderr, context);
			espeak_ng_ClearErrorContext(&context);
			return EXIT_FAILURE;
		}
		return EXIT_SUCCESS;
	}

	if (speed > 0)
		espeak_SetParameter(espeakRATE, speed, 0);
	if (pitch >= 0)
		espeak_SetParameter(espeakPITCH, pitch, 0);
	if (pitch_range >= 0)
		espeak_SetParameter(espeakRANGE, pitch_range, 0);
	if (wordgap >= 0)
		espeak_SetParameter(espeakWORDGAP, wordgap, 0);
	if (option_linelength > 0)
		espeak_SetParameter(espeakLINELENGTH, option_linelength, 0);

	espeak_SetPhonemeTrace(phoneme_options | (phonemes_separator << 8), f_phonemes_out);

	if (filename[0] == 0) {
		if ((optind < argc) && (flag_stdin == 0)) {
			p_text = argv[optind];
		} else {
			f_text = stdin;
			if (flag_stdin == 0)
				flag_stdin = 2;
		}
	} else {
		filesize = GetFileLength(filename);
		f_text = fopen(filename, "r");
		if (f_text == NULL) {
			fprintf(stderr, "Failed to read file '%s'\n", filename);
			exit(EXIT_FAILURE);
		}
	}

	if (p_text != NULL) {
		int size;
		size = strlen(p_text);
		espeak_Synth(p_text, size+1, 0, POS_CHARACTER, 0, synth_flags, NULL, NULL);
	} else if (flag_stdin) {
		size_t max = 1000;
		if ((p_text = (char *)malloc(max)) == NULL) {
			espeak_ng_PrintStatusCodeMessage(ENOMEM, stderr, NULL);
			exit(EXIT_FAILURE);
		}

		if (flag_stdin == 2) {
			while (fgets(p_text, max, f_text) != NULL) {
				p_text[max-1] = 0;
				espeak_Synth(p_text, max, 0, POS_CHARACTER, 0, synth_flags, NULL, NULL);
			}
			if (f_text != stdin) {
				fclose(f_text);
			}
		} else {
			ix = 0;
			while (true) {
				if ((c = fgetc(stdin)) == EOF)
					break;
				p_text[ix++] = (char)c;
				if (ix >= (int)(max-1)) {
					char *new_text = NULL;
					if (max <= SIZE_MAX - 1000) {
						max += 1000;
						new_text = (char *)realloc(p_text, max);
					}
					if (new_text == NULL) {
						free(p_text);
						espeak_ng_PrintStatusCodeMessage(ENOMEM, stderr, NULL);
						exit(EXIT_FAILURE);
					}
					p_text = new_text;
				}
			}
			if (ix > 0) {
				p_text[ix] = 0;
				espeak_Synth(p_text, ix, 0, POS_CHARACTER, 0, synth_flags, NULL, NULL);
			}
		}

		free(p_text);
	} else if (f_text != NULL) {
		if ((p_text = (char *)malloc(filesize+1)) == NULL) {
			espeak_ng_PrintStatusCodeMessage(ENOMEM, stderr, NULL);
			exit(EXIT_FAILURE);
		}

		fread(p_text, 1, filesize, f_text);
		p_text[filesize] = 0;
		espeak_Synth(p_text, filesize+1, 0, POS_CHARACTER, 0, synth_flags, NULL, NULL);
		fclose(f_text);

		free(p_text);
	}

	espeak_ng_Synchronize();

	if (f_phonemes_out != stdout)
		fclose(f_phonemes_out);

	espeak_ng_Terminate();
	return 0;
}
