#ifndef _write_SHAR_TALK_AUDIO_H
#define _write_SHAR_TALK_AUDIO_H
#include "StreDataType.h"
#include "audioType.h"
#include "shar_http.h"
#include "AudioDenoiser.h"
#include <algorithm> // 包含 std::find
#include <mutex>

// forward declare libfvad context
struct Fvad;

#define BUFF_SIZE 1024

class SharTalkAudio : public std::enable_shared_from_this<SharTalkAudio>
{
public:
    SharTalkAudio(const CONFIG ServerConfig);
    ~SharTalkAudio();

    bool sharInit(std::string sim, uint8_t loadType);
    bool write_shar_device(uint8_t *data, uint16_t size);
    void reint();
private:
    int ADPCM_decode(uint8_t *data, uint16_t size);
    int audio_decoder(uint8_t *data, uint16_t size);
    bool push_to_device(const uint8_t* pcm, int shortPcmSize, audioType& audioInfo);
    bool write_data(audioType& audioInfo, uint16_t BodyLen);
    void add_map(const std::string& sim, const std::string& groupId);
    void alter_map(audioType& audioInfo);
    uint64_t get_timestamp();
    bool isSpeechPresent(const short* pcm, int sampleCount);

    void appendPCMData(const uint8_t* pcm, size_t size);
    bool wsplayback(audioType& audioInfo, const uint8_t* pcm, int shortPcmSize);
    bool httpplayback(audioType& audioInfo, const uint8_t* pcm, int shortPcmSize);

public:
    std::string currentSIM;

private:
    uint8_t  audio_type;

    uint8_t* ucOutBuff;
    adpcm_state* deState;

    //write 
    adpcm_state* enState;
    uint8_t* audioEncodeBuf;

    sharHttpSer* sharHttSer;
    std::string groupID;
    int webSocketFd;
    int type;//1:群組對講, 2:AI對講
    uint8_t pkgCnt;

    bool isSpeaking;
    std::vector<uint8_t> wsRecvPcm;//ws接收的pcm数据缓存
    std::vector<uint8_t> httpRecvPcm;//http接收的pcm数据缓存
    uint32_t offset;
    std::mutex pcm_mutex;
    
    std::vector<uint8_t> pcmBuf;

    AudioDenoiser *pAudioDenoiser;
    Fvad* vad;

    ResponseHeader responseHeader;
    int64_t playingStartTime;//广告播放开始时间,用于统计广告播放时长
};

extern bool get_audio_type_info(std::string sim,audioType& audioInfo);
extern void del_audio_type_info(std::string sim);

#endif
