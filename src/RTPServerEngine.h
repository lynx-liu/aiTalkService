#ifndef _RTP_SERVER_ENGINE_H_
#define _RTP_SERVER_ENGINE_H_
#include "StreDataType.h"
#include "lock.h"
#include "../include/faac.h"

#include "audioType.h"
#include "writeSharTalkAudio.h"

#define DATA_TYPE_AUDIO  0x03
#define AUDIO_BUFF_SIZE 1024

class Caudio;
class CRTPServerEngine
{
public:
    CRTPServerEngine(const int fd, const CONFIG ServerConfig);
    ~CRTPServerEngine(); //virtual
    bool ReadAndAnalyzeRTPPack();
    void reInit();

private:
    void get_device_SIM(uint8_t* bcdSim, uint8_t bcdLen);
    bool insert_talk_info(const uint8_t* data, RTP_PKG_HEADER &header, uint8_t bcdLen);

private:
    int                     sockFd;
    std::string             m_BCDSIMStr;    //SIM号
    SharTalkAudio*          _sharTalkstrue;
};

extern bool add_audio_type_info(std::string sim,audioType AudtypeInfo);
extern void del_audio_type_info(std::string sim);

#endif
