#ifndef _RTP_SERVER_ENGINE_H_
#define _RTP_SERVER_ENGINE_H_
#include "StreDataType.h"
#include "md5.h"
#include <time.h>

//定时器
#include <stdio.h>
#include <signal.h>

#include "lock.h"
#include "../include/faac.h"

#include "audioType.h"
#include "rtmpPush.h"
#include "rtmpsender.h"
#include "converter.h"
#include "writeSharTalkAudio.h"

#define LOAD_TYPE_G711A  0x06   //G.711A
#define LOAD_TYPE_G726   0x08   //G.726
#define LOAD_TYPE_ADPCMA 0x1A
#define AUDIO_THREAD_STATUS_OFF 0
#define AUDIO_THREAD_STATUS_ON  1
#define AUDIO_DECODE_OUT_BUFF   1024
#define CREATE_AUDIO_STATUS_OFF 0
#define CREATE_AUDIO_STATUS_ON  1
#define CREATE_SIM_STATUS_OFF   0
#define CREATE_SIM_STATUS_ON    1

#define READ_RTP_PACK_MAXI 2048
#define READ_PACKET_MAXI 1024
#define SEND_VIDEO_DATA_MAXI 980
#define READ_PACKET_HEAD 31

#define READ_BUFF_MAXI 1024

#define DATA_TYPE_VIDE_I 0x00
#define DATA_TYPE_VIDE_P 0x01
#define DATA_TYPE_VIDE_B 0x02
#define DATA_TYPE_AUDIO  0x03
#define DATA_TYPE_TRANSM 0x04   //透传

#define _CONTROL_WRITE_STATUS_OFF 20
#define _CONTROL_WRITE_STATUS_ON  30

#define _MERGING_PACK_OFF 0
#define _MERGING_PACK_ON 1

#define SUB_PACK_MARKING_ATOMIC 0x00
#define SUB_PACK_MARKING_FIRST  0x01
#define SUB_PACK_MARKING_END    0x02
#define SUB_PACK_MARKING_MIDDLE 0x03

#define OBJECT_END_STATUS_OFF 0
#define OBJECT_END_STATUS_ON  1

#define RAW_AAC_BUFFER_SIZE   3*1024

#define MEMPOOL_INIT_STATUS_OFF 2
#define MEMPOOL_INIT_STATUS_ON 3
#define MEMPTR_SIZE 40*1024
#define RTMP_URL_INIT_STATUS_OFF 4
#define RTMP_URL_INIT_STATUS_ON 5
#define AUDIO_NEW_STATUS_OFF 6
#define AUDIO_NEW_STATUS_ON 7

#define AUDIO_BUFF_SIZE 1024
#define I_FIRST_STATUS 0
#define I_SECOND_STATUS 1

#define TALK_STATUS_FIST 0
#define TALK_STATUS_END 1

class Caudio;
class CRTPServerEngine
{
public:
    CRTPServerEngine(const CONFIG ServerConfig, uint8_t BCDSIMLength);
    ~CRTPServerEngine(); //virtual
    bool ReadAndAnalyzeRTPPack();
    void init(int fd);
    void reInit();

private:
    size_t ReadPackLen_g();
    bool RecvSocketFdDataPacket_g();
    bool AnalyzeHead(const unsigned char* headPack, const int& headLen);
    void AnalyzeHeadVideoEnd_14_g(const unsigned char* HeadPackEnd14);
    void AnalyzeHeadAudioEnd_10_g(const unsigned char* HeaPaAudioEnd10);
    void AnalyzeTransmissionHeadEnd_2_g(const unsigned char* HeaPaTranEnd2);

    bool CreateExeclFfmpegCourse();
    string MD5Encryption(const string& WaitEncryStr);
    string GetHushStreamAddEffectiveTime();

    void head_analysis_();
    void head_audio_analysis_();
    void head_penetrate_analysis_();
    void head_video_analysis_();
    void get_device_SIM_();
    void data_memcpy_();
    bool videAudioManage();
    bool sendDataV3();
    bool create_rtmpSendObj();
    void read_data_body();
    inline void read_packHead_ptr();
    inline void read_packHeadAfter_ptr();
    void start_init();

private:
    bool send_talk_Audio();
    bool insert_talk_info();
    bool push_aduio_data();

private:
    int  CsockFd;
    string urlDNS;
    string UrlKey;
    std::string baseUrl;
    ssize_t           RreadReturnLen;
    SAVER_RECV_DATA*  RecvRtpPackStr;
    std::string       RetTime_t;
	std::string       txSecret;
	std::string       EncryStr;
    std::string       StreamName;
    unsigned char     m_BCDSIMCard[16];
    char              m_TempBCDCard_t[8];
    std::string       m_BCDSIMStr;    //SIM号
    std::string       m_BCDChanStr;   //通道号
    unsigned char     channel_;
    int               SIMStatus;
    uint8_t*            _memptr;
    uint8_t           m_BCDSIMLength;
    uint16_t          m_HeadLen;
    

private:
    size_t  _size;
    size_t  gRecvLen;
    size_t     memsize;
    int        urlInitStatus;
    RtmpSender* rtmpSendObj;
    int    PackHeadLen_;
    int    PackStatus_;
    uint8_t  DataType4_;
    unsigned short WdBodyLen_;
    uint8_t* dataPtr;
    uint8_t* audioPtr;
    unsigned long  Bt8timeStamp;
    unsigned short _timeStamp;
    SEND_VIDEO_INFO_STRU AddToListData;
    SEND_VIDEO_INFO_STRU* dataStructPtr;
    
    int talkStatus;
    SharTalkAudio* _sharTalkstrue;
};


inline void CRTPServerEngine::read_packHead_ptr()
{
	dataPtr = RecvRtpPackStr->HeadPack + PackHeadLen_;
}

inline void CRTPServerEngine::read_packHeadAfter_ptr()
{
	dataPtr = RecvRtpPackStr->HeadAfter + PackHeadLen_;
}

extern bool add_audio_type_info(std::string sim,audioType AudtypeInfo);
extern void del_audio_type_info(std::string sim);

#endif
