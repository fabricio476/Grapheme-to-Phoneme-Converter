// Stub header - soundicon support removed for G2P-only build
#ifndef SOUNDICON_H
#define SOUNDICON_H

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
    return -1;
}

static inline int LoadSoundFile2(const char *fname) {
    (void)fname;
    return -1;
}

#endif
