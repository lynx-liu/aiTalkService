#ifndef _AUDIO_TYPE_H_
#define _AUDIO_TYPE_H_
#include "StreDataType.h"

struct audioType
{
    uint8_t         BCDSIMCardNumber[10];
    uint32_t        BCDSIMLen;
    uint8_t         ChannelNumber;
    uint8_t         type;
    uint32_t        socketFd;
    uint8_t         ADPCM_8[4];
    uint64_t        Bt8timeStamp;
    uint16_t        num;
};

#endif

