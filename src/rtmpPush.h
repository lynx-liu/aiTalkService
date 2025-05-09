#ifndef _RTMP_PUSH_H
#define _RTMP_PUSH_H
#include <cstdio>
#include <memory>
#include <iostream>
#include "StreDataType.h"
#include "splitter.h"
 
 extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
#include <libavutil/time.h>
#include <libavutil/timestamp.h>
#include <libavutil/opt.h>
};

#define SPS_PPS_BUFF_SIZE 1024
#define FIRST_SEND_STATUS_OF 0
#define FIRST_SEND_STATUS_ON 1
#define FIRST_NALU_TYPE_NON_SPS 2
#define FIRST_NALU_TYPE_YES_SPS 3

class rtmpPush
{
public:
    rtmpPush();
    virtual ~rtmpPush();

    bool init(char* url);
    bool executeProcess(SEND_VIDEO_INFO_STRU* gVideoInfoStru);
    bool executeProcess2(SEND_VIDEO_INFO_STRU* gVideoInfoStru);

private:
    bool openInput();
    bool openOutput();
    int  readAndWrite();
    void CloseInput();
    void CloseOutput();
    static int read_Callback(void *opaque, uint8_t *buf, int buf_size);
    bool sendVideoData2(uint8_t* pNalu, Cnvt::USHORT nNaluSize);
    int interrupt_cb(void *ctx);
    shared_ptr<AVPacket> ReadPacketFromSource();
    void av_packet_rescale_ts(AVPacket *pkt, AVRational src_tb, AVRational dst_tb);
    int WritePacket(shared_ptr<AVPacket> packet);

private:
    AVOutputFormat*  ofmt;
    AVFormatContext* ofmt_ctx;

    AVPacket pkt;
    int label;
    int videoindex;
    int frame_index;
    long start_time;
    AVFormatContext* outputContext;
    AVFormatContext* inputContext;
    Cnvt::ULONG      lastReadPacktTime;
    int stream_index;
    int waitI;
    int rtmpisinit;
    int ptsInc;

    std::string rtmpUrl;
    int   spsStatus;

private:
    uint8_t* spsBuff;
    uint8_t* ppsBuff;
    int      spsLen;
    int      ppsLen;
    int      naluType;
    int      status;
    Cnvt::uint8_t* inputBuff;
	Cnvt::USHORT inputLen;
	Cnvt::ULONG  Bt8timeStamp;
    Cnvt::ULONG  startTimeStamp;
	Cnvt::USHORT _CompositionTime;   
};

#endif