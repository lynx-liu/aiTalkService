#ifndef _AUDIO_TYPE_H_
#define _AUDIO_TYPE_H_
#include "StreDataType.h"

struct audioType
{
    unsigned char       BCDSIMCardNumber[10];
    int                 BCDSIMLen;
    unsigned char       ChannelNumber;
    unsigned char		Tag_PayloadType;
    int                 socketFd;
    unsigned char       ADPCM_8[16];
    unsigned long int   Bt8timeStamp;
    int                 num;
    adpcm_state*        adpcmState;
    char                 index;
};

#endif

