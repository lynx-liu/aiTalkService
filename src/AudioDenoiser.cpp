#include "AudioDenoiser.h"
#include <algorithm>  // for std::max / std::min

AudioDenoiser::AudioDenoiser(int sampleRate, int mode) {
    sampleRateHz = sampleRate;
    frameSamples = sampleRateHz / 100; // 每帧 = 10ms
    nsHandle = WebRtcNs_Create();
    WebRtcNs_Init(nsHandle, sampleRateHz);
    WebRtcNs_set_policy(nsHandle, mode);  // 降噪等级：0~2
}

AudioDenoiser::~AudioDenoiser() {
    WebRtcNs_Free(nsHandle);
}

void AudioDenoiser::denoiseBuffer(short* pcm, int sampleCount) {
    float in[480] = {0}, out[480] = {0}; // 最大支持 48kHz（480）
    const float* in_ptrs[1] = { in };
    float* out_ptrs[1] = { out };

    for (int i = 0; i + frameSamples <= sampleCount; i += frameSamples) {
        // short -> float
        for (int j = 0; j < frameSamples; ++j) {
            in[j] = static_cast<float>(pcm[i + j]);
        }

        // 分析噪声
        WebRtcNs_Analyze(nsHandle, in);

        // 降噪处理
        WebRtcNs_Process(nsHandle, in_ptrs, 1, out_ptrs);

        // float -> short
        for (int j = 0; j < frameSamples; ++j) {
            float val = out[j];
            val = std::max(-32768.f, std::min(32767.f, val)); // 防止溢出
            pcm[i + j] = static_cast<short>(val);
        }
    }
}
