#ifndef _STRE_DATA_TYPE_H_
#define _STRE_DATA_TYPE_H_
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <errno.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <pthread.h>
#include<sys/wait.h>
#include <map>
#include <list>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "config.h"

#include <semaphore.h>
#include <sys/time.h>
#include<queue>
#include "adpcm.h"
#include "g711.h"

//消息队列
#include <sys/ipc.h>
#include <sys/msg.h>
#define MESSGE_QUEUE_SEND_TYPE 1
#define MESSGE_QUEUE_READ_TYPE 2
#define MESSGE_QUEUE_KEY 12345
#define MESSGE_QUEUE_TYPE      1              //发送消息队列类型

//读取保存包状态宏
#define _PKG_HD_INIT                0          //初始状态，准备接收数据包头前15字节
#define _PKG_HD_RECVING             1          //接收包头中，包头不完整，继续接收中
#define _PKG_HD_REMAINING_INIT_1    2          //后续包头字节
#define _PKG_HD_REMAINING_INIT_2    3
#define _PKG_HD__REMAINING_RECVING  5          //接收报文
#define _PKG_HD_REMAINING_INIT_ERR  404

#define _PKG_BD_INIT                6          //包头刚好收完，准备接收包体
#define _PKG_BD_RECVING             7          //接收包体中，包体不完整，继续接收中，处理后直接回到_PKG_HD_INIT状态
#define _PKG_RV_FINISHED            8          //完整包收完

//是否创建Ffmpeg状态宏
#define _EXECL_CREATE_STATUS_OFF    0          //没有创建ffmpeg进程
#define _EXECL_CREATE_STATUS_ON     1            //已经创建ffmpeg进程

//开辟一个堆区存放视频数据宏状态
#define _NEW_HEAP_STATUS_OFF        0           //未开辟一个空间
#define _NEW_HEAP_STATUS_ON         1           //已开辟一个空间

//定时器 时间宏 1小时删除不用进程
#define _TIMER_KILL_TIME_           60     //秒

#define MSG_SERVER_TYPE 3
#define MSG_CLIENT_TYPE 2


//管道读写宏
#define IN  0
#define OUT 1
using namespace std;
class RtpServer;
class CRTPServerEngine;

/***************************************
 结构类型说明：RTP包数据结构
 **************************************/
#pragma pack (1)
typedef struct _RTP_PKG_HEADER
{
    uint32_t            DWFramHeadMark;          //帧头标识
    unsigned char       V2:2;                    //固定为2
    unsigned char       P1:1;                    //固定为0
    unsigned char       X1:1;                    //RTP头是否需要扩展位，固定为0
    unsigned char       CC4:4;                   //固定为1
    unsigned char       M1:1;                    //标志位，确定是否是完整数据帧的边界
    unsigned char       PT7:7;                   //负载类型
    unsigned short      WdPackageSequence;        //RTP数据包序号每发送一个RTP数据包序列号加1
    unsigned char       BCDSIMCardNumber[10];     //SIM卡号BCDSIMCardNumber[10];
    unsigned char       Bt1LogicChannelNumber;     //逻辑通道号
    unsigned char       DataType4:4;                //数据类型
    unsigned char       subpackageHandleMark4:4;    //分包处理标记
    unsigned long int   Bt8timeStamp;              //时间戳
    unsigned short      WdLastIFrameInterval;      //与上一帧的时间间隔
    unsigned short      WdLastFrameInterval;       //与上一帧的时间间隔
    unsigned short      WdBodyLen;                 //数据体长度

}RTP_PKG_HEADER,*PRTP_PKG_HEADER;

typedef struct _RTP_PACKET_HEAD
{
    unsigned            DWFramHeadMark;          //帧头标识
    unsigned char       V2:2;                    //固定为2
    unsigned char       P1:1;                    //固定为0
    unsigned char       X1:1;                    //RTP头是否需要扩展位，固定为0
    unsigned char       CC4:4;                   //固定为1
    unsigned char       M1:1;                    //标志位，确定是否是完整数据帧的边界
    unsigned char       PT7:7;                      //负载类型
    unsigned short      WdPackageSequence;           //RTP数据包序号每发送一个RTP数据包序列号加1
    unsigned char       BCDSIMCardNumber[10];       //SIM卡号
    unsigned char       Bt1LogicChannelNumber;     //逻辑通道号
    unsigned char       DataType4:4;                //数据类型
    unsigned char       subpackageHandleMark4:4;    //分包处理标记
}PACKET_HEAD;

/***************************************
 结构类型说明：包头后10字节结构体
 **************************************/
typedef struct _RTP_PACKET_HEAD_10
{
    unsigned long int   Bt8timeStamp;               //时间戳
    unsigned short      WdBodyLen;                  //数据体长度
}PACKET_HEAD_10;

/***************************************
 结构类型说明：接收解析后视频数据包
 **************************************/
typedef struct SavRecvData
{
    int            PackStatus;                        //包状态
    unsigned char  HeadPack[30];            //存放前16字节
    unsigned char  HeadAfter[16];           //后14字节(最大14字节, 最小2字节)
    int            PackHeadLen;                     
    RTP_PKG_HEADER PKG_HEADER;
    int            NewHeapStatus;            //开辟一个堆区状态   _NEW_HEAP_STATUS_OFF -未开启 _NEW_HEAP_STATUS_ON -已开辟
    unsigned char* VideoData;                //视频报文数据最大为950字节
}SAVER_RECV_DATA;

//11/25 (父进程使用结构)
/***************************************
 结构类型说明：发送到ffmpeg子进程数据信息
 **************************************/
typedef struct SendVideoInfoList
{
    unsigned short  WdBodyLen;
    unsigned char*  VidePacData;
    unsigned long   Bt8timeStamp;
    unsigned short  timeStamp;
    size_t          memsize;
}SEND_VIDEO_INFO_STRU;
#pragma pack()

#endif
