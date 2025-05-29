#ifndef _STRE_DATA_TYPE_H_
#define _STRE_DATA_TYPE_H_

#include <sys/epoll.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <queue>
#include "adpcm.h"
#include "g711.h"
#include "config.h"

#define LOAD_TYPE_G711A 0x06 // G.711A
#define LOAD_TYPE_G726 0x08  // G.726
#define LOAD_TYPE_ADPCM 0x1A // ADPCM

inline uint64_t htonll(uint64_t val) {return (((uint64_t)htonl(val))<<32)+htonl(val>>32);}
inline uint64_t ntohll(uint64_t val) {return (((uint64_t)ntohl(val))<<32)+ntohl(val>>32);}

/***************************************
 结构类型说明：RTP包数据结构
 **************************************/
#pragma pack(1)
typedef struct _RTP_PKG_HEADER
{
    uint32_t DWFramHeadMark = 0x30316364; // 帧头标识
    /*
    uint8_t     V2:2;                    //固定为2
    uint8_t     P1:1;                    //固定为0
    uint8_t     X1:1;                    //RTP头是否需要扩展位，固定为0
    uint8_t     CC4:4;                   //固定为1
    */
    uint8_t flag = 0x81; // 固定0x81
    /*
    uint8_t     M1:1;                    //标志位，确定是否是完整数据帧的边界
    uint8_t     PT7:7;                   //负载类型
    */
    uint8_t type;

    uint16_t WdPackageSequence;    // RTP数据包序号每发送一个RTP数据包序列号加1
    uint8_t BCDSIMCardNumber[10];  // SIM卡号BCDSIMCardNumber[10];
    uint8_t Bt1LogicChannelNumber; // 逻辑通道号

    /*
    uint8_t DataType4 : 4;             // 数据类型
    uint8_t subpackageHandleMark4 : 4; // 分包处理标记
    */
    uint8_t info = 0x30;

    uint64_t Bt8timeStamp; // 时间戳
    uint16_t WdBodyLen;    // 数据体长度

} RTP_PKG_HEADER, *PRTP_PKG_HEADER;

#pragma pack()

#endif
