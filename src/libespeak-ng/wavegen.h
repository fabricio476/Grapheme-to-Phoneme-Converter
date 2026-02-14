// Stub header - wavegen removed for G2P-only build
#ifndef WAVEGEN_H
#define WAVEGEN_H

#include <stdint.h>

// Forward declaration
typedef struct { // minimal forward compat
	char v_name[40];
} voice_t_stub;

// Stub declarations
extern int samplerate;

// These are now no-ops for G2P
void WavegenInit(int rate, int wavemult);
void WavegenFill(void);
void WavegenFini(void);
int WcmdqUsed(void);
void WcmdqStop(void);
void WcmdqInc(void);
void GetAmplitude(void);
void InitBreath(void);

#endif
