// Stub header — audio synthesis stubs for G2P-only build.
// Merged from: mbrola.h, wavegen.h, soundicon.h
#ifndef ESPEAK_NG_STUBS_H
#define ESPEAK_NG_STUBS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

// =================================================================
// Wavegen stubs (was wavegen.h)
// =================================================================

extern int samplerate;

void GetAmplitude(void);
void InitBreath(void);

// =================================================================
// MBROLA stubs (was mbrola.h)
// =================================================================

extern char mbrola_name[20];

static inline int LoadMbrolaTable(const char *mbrola_voice, const char *phtrans, int *srate) {
    (void)mbrola_voice; (void)phtrans; (void)srate;
    return 0;
}

// =================================================================
// Sound icon stubs (was soundicon.h)
// =================================================================

typedef struct {
    int name;
    char *filename;
    int length;
    int data;
} SOUND_ICON;

#define N_SOUNDICON_TAB 100
extern SOUND_ICON soundicon_tab[N_SOUNDICON_TAB];
extern int n_soundicon_tab;

static inline int LookupSoundicon(int c) {
    (void)c;
    return -1; // no sound icons in G2P-only build
}

static inline int LoadSoundFile2(const char *fname) {
    (void)fname;
    return -1; // no sound files in G2P-only build
}

#ifdef __cplusplus
}
#endif

#endif
