#ifndef G726_PRIVATE_H_
#define G726_PRIVATE_H

#include "g72x.h"

int linear2ulaw(int pcm_val);
int ulaw2linear(int u_val);
int linear2alaw(int pcm_val);
int alaw2linear(int u_val);

int predictor_zero(g726_state *state_ptr);
int predictor_pole(g726_state *state_ptr);
int step_size(g726_state *state_ptr);
int quantize(int d, int y, int *table, int size);
int reconstruct(int sign, int dqln, int y);
void update(int code_size, int y, int wi, int fi, int dq, int sr, int dqsez,
            g726_state *state_ptr);
int tandem_adjust_alaw(int sr, int se, int y, int i, int sign, int *qtab);
int tandem_adjust_ulaw(int sr, int se, int y, int i, int sign, int *qtab);

#endif
