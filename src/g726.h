#ifndef _G726_H_
#define _G726_H_

#include "g72x.h"

typedef struct {
    g726_state enc;
    g726_state dec;
} g726_codec_state;

#ifdef __cplusplus
extern "C" {
#endif

void g726_codec_init(g726_codec_state *state);

/* G.726-32 (4bit/sample), RFC3551 little-endian nibble packing */
int g726_encode_32(g726_codec_state *state, const short *pcm, int sample_count,
                   unsigned char *out);
int g726_decode_32(g726_codec_state *state, const unsigned char *data, int byte_count,
                   short *pcm_out);

#ifdef __cplusplus
}
#endif

#endif
