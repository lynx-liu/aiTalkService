#include "g726.h"

void g726_codec_init(g726_codec_state *state)
{
    if (!state)
        return;
    g726_init_state(&state->enc);
    g726_init_state(&state->dec);
}

int g726_encode_32(g726_codec_state *state, const short *pcm, int sample_count,
                   unsigned char *out)
{
    int out_bytes = 0;
    int i;

    if (!state || !pcm || !out || sample_count <= 0)
        return 0;

    for (i = 0; i < sample_count; i += 2) {
        int code0 = g726_32_encoder(pcm[i], AUDIO_ENCODING_LINEAR, &state->enc) & 0x0f;
        int code1 = 0;

        if (i + 1 < sample_count)
            code1 = g726_32_encoder(pcm[i + 1], AUDIO_ENCODING_LINEAR, &state->enc) & 0x0f;

        out[out_bytes++] = (unsigned char)((code1 << 4) | code0);
    }

    return out_bytes;
}

int g726_decode_32(g726_codec_state *state, const unsigned char *data, int byte_count,
                   short *pcm_out)
{
    int samples = 0;
    int i;

    if (!state || !data || !pcm_out || byte_count <= 0)
        return 0;

    for (i = 0; i < byte_count; i++) {
        unsigned char b = data[i];
        int code0 = b & 0x0f;
        int code1 = (b >> 4) & 0x0f;

        pcm_out[samples++] = (short)g726_32_decoder(code0, AUDIO_ENCODING_LINEAR, &state->dec);
        pcm_out[samples++] = (short)g726_32_decoder(code1, AUDIO_ENCODING_LINEAR, &state->dec);
    }

    return samples;
}
