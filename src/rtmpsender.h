#ifndef RTMPSENDER_H
#define RTMPSENDER_H
#include <iostream>
#include <string.h>
#include <stdio.h>
#include "StreDataType.h"
#include "splitter.h"

#include "librtmp/rtmp.h"

#define BODY_SIZE 1024
#define MAX_BUFF_SIZE 409600
#define SPS_PPS_BUFF_SIZE 1024
#define SPS_DECODER_STATUS_OF 0 
#define SPS_DECODER_STATUS_ON 1
#define PPS_DECODER_STATUS_OF 2 
#define PPS_DECODER_STATUS_ON 3 
#define DELIMITER_SIZE_STATUS_OF 4  
#define DELIMITER_SIZE_STATUS_ON 5  
#define INIT_STATUS_OF 6
#define INIT_STATUS_ON 7

#define PACKET_BUFF_SIZE 40*1024
#define PPS_SPS_PACKET_ALLOC_SIZE 2048
class RtmpSender
{
public:
    RtmpSender();
    ~RtmpSender();
    bool init(char* rtmpUrl, SEND_VIDEO_INFO_STRU* dataPtr);
    bool executeProcess(/*SEND_VIDEO_INFO_STRU* gVideoInfoStru*/);
    void close_free();
    void reInit();
private:
    int sendH264Frame(/*Cnvt::uint8_t*, int, Cnvt::UINT, Cnvt::UINT*/);
    int SendPacket(Cnvt::UINT,Cnvt::uint8_t*,Cnvt::UINT,Cnvt::UINT);
    int SendVideoSpsPps(/*Cnvt::uint8_t *ppsptr,int pps_len,Cnvt::uint8_t* spsptr,int sps_len, Cnvt::UINT _TimeStamp*/);

    bool sendVideoData(/*Cnvt::uint8_t* pNalu, Cnvt::USHORT nNaluSize*/);
    bool sps(/*Cnvt::uint8_t* pNalu, Cnvt::USHORT nNaluSize*/);
    bool pps(/*Cnvt::uint8_t* pNalu, Cnvt::USHORT nNaluSize*/);

private:
    struct RTMP* m_pRtmp;
    Cnvt::uint8_t* body;
    Cnvt::uint8_t* spsBuff;
    Cnvt::uint8_t* ppsBuff;

    int      spsLen;
    int      ppsLen;

    Cnvt::uint8_t* spsptr;
    Cnvt::uint8_t* ppsptr;
    int sps_len;
    int pps_len;

    Cnvt::UINT nTimeStamp;

    int     spsStatus;
    int     ppsStatus;

    int     naluType;

private:
    SEND_VIDEO_INFO_STRU* gVideoInfoStru;
    Cnvt::uint8_t* inputBuff;
	Cnvt::USHORT inputLen;
	Cnvt::ULONG  Bt8timeStamp;
	Cnvt::USHORT _CompositionTime;
    Cnvt::ULONG  startTime;
    Cnvt::USHORT nNaluSize;
    Cnvt::uint8_t* pNalu;
    int          nOffset;

    RTMPPacket* packet;
    int         packSize;

    RTMPPacket* packet2;
    int         initStatus;
};

#endif //RTMP_SENDER