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

#define VIDEO_DATA_MAXI        1024            //视频数据最大值

//读取保存包状态宏
#define _PKG_HD_INIT                0          //初始状态，准备接收数据包头前15字节
#define _PKG_HD_RECVING             1          //接收包头中，包头不完整，继续接收中
#define _PKG_HD_REMAINING_INIT_1    2          //后续包头字节
#define _PKG_HD_REMAINING_INIT_2    3
#define _PKG_HD_REMAINING_INIT_3    4
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
struct  RTPpacket{
    unsigned int FrameHeadFlag;
    unsigned char v:2;
    unsigned char p:1;
    unsigned char x:1;
    unsigned char cc:4;
    unsigned char M:1;
    unsigned char PT:7;
    unsigned short PacketNum;
    unsigned char SIM[6];
    unsigned char channel;
    unsigned char DataType:4;
    unsigned char SubPackageFlag:4;
    unsigned long int timestamp;
    unsigned short LastIFrameIn;
    unsigned short LastFrameIn;
    unsigned short DataLeng;
};

/***************************************
 结构类型说明：RTP包数据结构
 **************************************/
#pragma pack (1)
typedef struct _RTP_PKG_HEADER
{
//    unsigned short pkgLen;              //报文总长度--2字节
//    unsigned short msgCode;             //消息类型代码--2字节,用于区别每个不同的命令
//    int            crc32;               //CRC32效验--4字节,为了防止收发数据中出现收到内容和发送内容不一致的情况
    unsigned            DWFramHeadMark;          //帧头标识
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
#pragma pack()

#pragma pack (1)
typedef struct _RTP_PKG_HEADER_SIM6
{
//    unsigned short pkgLen;              //报文总长度--2字节
//    unsigned short msgCode;             //消息类型代码--2字节,用于区别每个不同的命令
//    int            crc32;               //CRC32效验--4字节,为了防止收发数据中出现收到内容和发送内容不一致的情况
    unsigned            DWFramHeadMark;          //帧头标识
    unsigned char       V2:2;                    //固定为2
    unsigned char       P1:1;                    //固定为0
    unsigned char       X1:1;                    //RTP头是否需要扩展位，固定为0
    unsigned char       CC4:4;                   //固定为1
    unsigned char       M1:1;                    //标志位，确定是否是完整数据帧的边界
    unsigned char       PT7:7;                   //负载类型
    unsigned short      WdPackageSequence;        //RTP数据包序号每发送一个RTP数据包序列号加1
    unsigned char       BCDSIMCardNumber[6];     //SIM卡号BCDSIMCardNumber[10];
    unsigned char       Bt1LogicChannelNumber;     //逻辑通道号
    unsigned char       DataType4:4;                //数据类型
    unsigned char       subpackageHandleMark4:4;    //分包处理标记
    unsigned long int   Bt8timeStamp;              //时间戳
    unsigned short      WdLastIFrameInterval;      //与上一帧的时间间隔
    unsigned short      WdLastFrameInterval;       //与上一帧的时间间隔
    unsigned short      WdBodyLen;                 //数据体长度

}RTP_PKG_HEADER_SIM6,*PRTP_PKG_HEADER_SIM6;
#pragma pack()

/***************************************
 结构类型说明：16字节包头结构体
 **************************************/
//#pragma pack (1)
typedef struct _RTP_PACKET_HEAD_16
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
}PACKET_HEAD_16;
//#pragma pack()


#pragma pack (1)
typedef struct _RTP_PACKET_HEAD_SIM6
{
    unsigned            DWFramHeadMark;          //帧头标识
    unsigned char       V2:2;                    //固定为2
    unsigned char       P1:1;                    //固定为0
    unsigned char       X1:1;                    //RTP头是否需要扩展位，固定为0
    unsigned char       CC4:4;                   //固定为1
    unsigned char       M1:1;                    //标志位，确定是否是完整数据帧的边界
    unsigned char       PT7:7;                      //负载类型
    unsigned short      WdPackageSequence;           //RTP数据包序号每发送一个RTP数据包序列号加1
    unsigned char       BCDSIMCardNumber[6];       //SIM卡号
    unsigned char       Bt1LogicChannelNumber;     //逻辑通道号
    unsigned char       DataType4:4;                //数据类型
    unsigned char       subpackageHandleMark4:4;    //分包处理标记
}PACKET_HEAD_SIM6;
#pragma pack()

/***************************************
 结构类型说明：包头后14字节结构体
 **************************************/

typedef struct _RTP_PACKET_HEAD_14
{
    unsigned long int   Bt8timeStamp;              //时间戳
    unsigned short      WdLastIFrameInterval;      //与上一帧的时间间隔
    unsigned short      WdLastFrameInterval;       //与上一帧的时间间隔
    unsigned short      WdBodyLen;                 //数据体长度
}PACKET_HEAD_14;

/***************************************
 结构类型说明：包头后10字节结构体
 **************************************/
typedef struct _RTP_PACKET_HEAD_10
{
    unsigned long int   Bt8timeStamp;               //时间戳
    unsigned short      WdBodyLen;                  //数据体长度
}PACKET_HEAD_10;


/***************************************
 结构类型说明：执行推流ffmpeg进程信息
 **************************************/
typedef struct FfmpegCourseInfo
{
    int    run;
    int    PipeFd[2];                                
}FFMPEG_COU_INFO;

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

typedef struct SavRecvData_SIM6
{
    int            PackStatus;                        //包状态
    unsigned char  HeadPack[30];            //存放前16字节
    unsigned char  HeadAfter[16];           //后14字节(最大14字节, 最小2字节)
    int            PackHeadLen;                     
    RTP_PKG_HEADER_SIM6 PKG_HEADER;
    int            NewHeapStatus;            //开辟一个堆区状态   _NEW_HEAP_STATUS_OFF -未开启 _NEW_HEAP_STATUS_ON -已开辟
    unsigned char* VideoData;                //视频报文数据最大为950字节
}SAVER_RECV_DATA_SIM6;


/*********
*共享对讲音频数据结构
*********/
typedef struct sharTalkDataType
{
    unsigned char  PackHead[16];
    // unsigned char  PackAfter[16];
    int            PackHeadLen;
    // int            packAfterLen;
    unsigned short      WdBodyLen;                 //数据体长度
}SHAR_TALK_DATA_TYPE;

#pragma pack (1)
typedef struct FramHeadPack
{
    unsigned char       FramHead8[8];          //帧头8字节
    unsigned char       BCDSIMCardNumber[10];     //SIM卡号BCDSIMCardNumber[10];
    unsigned char       Bt1LogicChannelNumber;     //逻辑通道号
//    unsigned char       DataType4:4;                //数据类型     0011：音频帧
//    unsigned char       subpackageHandleMark4:4;    //分包处理标记    0000：原子包，不可被拆分
    unsigned char        DataType4Label;       //0011 0000 = 0X30
//	unsigned char		FramAf[10];
    unsigned long int   Bt8timeStamp;              //时间戳
    unsigned short      WdBodyLen;                 //数据体长度

}FRAM_HEADER;
#pragma pack()



/***************************************
 结构类型说明：父子进程消息队列发送接收结构体
 **************************************/
typedef struct PackSdtreamInfo
{
//    unsigned char  BCDSIMCard[7];              
//    unsigned char  DeviChannel;                    //设备通道号
    unsigned short WdBodyLen;                      //
    unsigned char  VidePacData[VIDEO_DATA_MAXI]; 
}PACK_STREAM_INFO;

//消息队列device
/***************************************
 结构类型说明：通过消息队列发送数据到子进程
 **************************************/
#define MSGKEY 12345
typedef struct StreamMessageQueue
{
	long msgtype;
	PACK_STREAM_INFO MsgData;
}STREAM_MSG_QUEUE;

/***************************************
 结构类型说明：创建子进程使用进程信息
 **************************************/
typedef struct SaveStreamInfoMap
{
    // int            Gindex;
    // pthread_cond_t Cond;
    char           txSecret[64];                         //开启推流鉴权后生成的鉴权串
    char           txTime[16];                           //十六进制推流有效时间 (如:5DD435ED)
    char           StreamName[16];
    char           BCDSIMCard[16];
    char           DeviChannel[5];
    char           urlDNS[128];
    int            FfmpegPidFd[2];                 //进程管道ID
    int            ExeclCreStatus;                //判断是否已经创建Ffmpeg进程  0-否   1-是  
    int            FfmpegPid_t;                    //子进程ID
    char           MkfifoNameStr[64];             //有名管道地址名
    char           gUUIDstr[36];
}SAVE_STREAM_INFO;

/***************************************
 结构类型说明：存储MSG接收的消息结构(MAP)
 **************************************/
typedef struct SaveMsgInfoMap
{
    unsigned short WdBodyLen;
    unsigned char* VidePacData;
}SAVE_MSG_INFO_MAP;
/***************************************
 结构类型说明：发送状态控制结构信息(map)
 **************************************/
typedef struct SendStatusInfoMap
{
    int          ExeclCreStatus;                //判断是否已经创建Ffmpeg进程  0-否   1-是
    int          FfmpegPidFd[2];                 //进程管道ID
    time_t       CreateTime;                     //线程创建时间
    int          FfmpegPid_t;                    //子进程ID
}SEND_STATUS_INFO_MAP;



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

//11/25(父进程使用结构)
/***************************************
 结构类型说明：发送状态控制结构信息(map)
 **************************************/
typedef std::list<SEND_VIDEO_INFO_STRU*> VIDEO_LIST;
typedef struct VideoStatusInfoMap
{
    pthread_cond_t   Cond;
    int              ExeclCreStatus;                //判断是否已经创建Ffmpeg进程  0-否   1-是
    time_t           CreateTime;                     //线程创建时间
    time_t           ActualSendTime;                 //实时发送到ffmpeg子进程时间， 超过1小时进程退出
    pid_t            FfmpegPid_t;                    //子进程ID
    pthread_t        pthreadId;
    int              OpenMkfifoFd1;                   //子进程打开的fd
    int              OpenMkfifoFd2;                  //打开有名管道fd
    char             MkfifoName[64];
    VIDEO_LIST       VideoList;

    int              Gmsgid;
}VIDEO_STATUS_INFO_MAP;


typedef std::list<SEND_VIDEO_INFO_STRU> DATA_LIST;
typedef struct DataInfoStruct
{
    // pthread_cond_t   Cond;
    int              ExeclCreStatus;                
    time_t           CreateTime;                     
    time_t           ActualSendTime;                
    pid_t            FfmpegPid_t;                    
    pthread_t        pthreadId;
    int              OpenMkfifoFd1;                  
    int              OpenMkfifoFd2;                  
    char             MkfifoName[64];
    DATA_LIST       DataList;

    int              Gmsgid;
}DATA_INFO_STRUCT;


/***************************************
 结构类型说明：epoll 事件结构
 **************************************/
typedef struct EpollEvents
{
    int                 gSocketFd;
    int                 gEpollFd;
    CRTPServerEngine*   gDevObjStram;
    struct EpollEvents* EpollEvenPtr;
}_EPOLL_EVENTS;

/***************************************
 结构类型说明：消息队列结构
 **************************************/
typedef struct msgbuff
{
    long  mtype;
    char  status[2];
}_MSG_BUFF;


//20.06.15
/***************************************
 结构类型说明：设备管理信息MAP
 **************************************/
typedef struct VideoInfoMap
{
    pid_t  FfmpegPid_t;                   
    int    Gmsgid;
    FILE*  Gfp;
}VIDEO_INFO_MAP;

/***************************************
 结构类型说明：创建子进程信息
 **************************************/
typedef struct ChanStreamInfo
{
    int            Gindex;
    char           txSecret[64];                         //开启推流鉴权后生成的鉴权串
    char           txTime[16];                           //十六进制推流有效时间 (如:5DD435ED)
    char           StreamName[16];
    char           urlDNS[128];
}CHAN_STREAM_INFO;


typedef struct MergingPackInfo
{
    int                    status;
    SEND_VIDEO_INFO_STRU*  VideoInfoPtr;
}MERG_PACK_INFO;


typedef struct AudioInfo
{
    char           txSecret[64];                         //开启推流鉴权后生成的鉴权串
    char           txTime[16];                           //十六进制推流有效时间 (如:5DD435ED)
    char           StreamName[16];
    char           BCDSIMCard[16];
    char           DeviChannel[5];
    char           urlDNS[128];
    int            FfmpegPidFd[2];                 //进程管道ID
    int            FfmpegPid_t;                    //子进程ID
    pthread_t      pthread_pid;
}AUDIO_STRUCT_INFO;


#endif