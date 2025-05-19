#include "noise_suppression.h"

class AudioDenoiser {
    NsHandle* nsHandle;
    int frameSamples;
    int sampleRateHz;

public:
    AudioDenoiser(int sampleRate, int mode = 2);
    ~AudioDenoiser();
    void denoiseBuffer(short* pcm, int sampleCount);
};
