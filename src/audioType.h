#ifndef _AUDIO_TYPE_H_
#define _AUDIO_TYPE_H_
#include "StreDataType.h"

// 对讲类型(按位标志, 可组合)。TYPE_WS_WEB_TALK 优先级最高
#define TYPE_GROUP_TALK     0x01 //群组对讲
#define TYPE_AI_TALK        0x02 //AI对讲
#define TYPE_WS_VAR_TALK    0x04 //多变量通知触发的对讲
#define TYPE_WS_WEB_TALK    0x08 //通过websocket平台对讲(ws以非json形式连接)

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

