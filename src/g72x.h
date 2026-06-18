#ifndef _G72X_H_
#define _G72X_H_

#define AUDIO_ENCODING_ULAW (1)
#define AUDIO_ENCODING_ALAW (2)
#define AUDIO_ENCODING_LINEAR (3)

typedef struct g726_state_s {
    long yl;
    int yu;
    int dms;
    int dml;
    int ap;
    int a[2];
    int b[6];
    int pk[2];
    short dq[6];
    int sr[2];
    int td;
} g726_state;

void g726_init_state(g726_state *state_ptr);

int g726_32_encoder(int sample, int in_coding, g726_state *state_ptr);
int g726_32_decoder(int code, int out_coding, g726_state *state_ptr);

#endif
