#ifndef _RTP_SERVER_ENGINE_H_
#define _RTP_SERVER_ENGINE_H_
#include <memory>
#include "StreDataType.h"
#include "lock.h"
#include "../include/faac.h"

#include "audioType.h"
#include "writeSharTalkAudio.h"
#include "RtmpSender.h"

#define DATA_TYPE_VIDE_I 0x00
#define DATA_TYPE_VIDE_P 0x01
#define DATA_TYPE_VIDE_B 0x02
#define DATA_TYPE_AUDIO  0x03
#define DATA_TYPE_TRANSM 0x04

enum PackageFlag : uint8_t {
    PKG_FLAG_ATOM   = 0x0,
    PKG_FLAG_FIRST  = 0x1,
    PKG_FLAG_LAST   = 0x2,
    PKG_FLAG_MIDDLE = 0x3
};

#define BUFF_SIZE 1024

class Caudio;
class CRTPServerEngine
{
public:
    CRTPServerEngine(const int fd, const CONFIG ServerConfig);
    ~CRTPServerEngine(); //virtual
    void ReadAndAnalyzeRTPPack();

private:
    int getBcdLen(uint8_t* bcdSim, int DataType, int subpackageHandleMark);
    void get_device_SIM(uint8_t* bcdSim, uint8_t bcdLen);
    bool insert_talk_info(const uint8_t* data, RTP_PKG_HEADER &header, uint8_t bcdLen);
    std::string getPushTimeHexString();
    std::string getMD5(const std::string& str);

private:
    int                     sockFd;
    std::string             m_BCDSIMStr;    //SIM号
    std::shared_ptr<SharTalkAudio> _sharTalkstrue;
};

extern bool add_audio_type_info(std::string sim,audioType AudtypeInfo);
extern void del_audio_type_info(std::string sim);

#endif
