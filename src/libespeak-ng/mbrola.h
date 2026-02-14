// Stub header - mbrola support removed for G2P-only build
#ifndef MBROLA_H
#define MBROLA_H

#include <stdbool.h>

// Forward declare PHONEME_LIST to avoid circular includes
struct PHONEME_LIST;

extern char mbrola_name[20];
extern int mbrola_delay;

static inline int LoadMbrolaTable(const char *mbrola_voice, const char *phtrans, int *srate) {
    (void)mbrola_voice; (void)phtrans; (void)srate;
    return 0;
}

static inline int MbrolaGenerate(struct PHONEME_LIST *phoneme_list, int *n_ph, bool resume) {
    (void)phoneme_list; (void)n_ph; (void)resume;
    return 0;
}

#endif
