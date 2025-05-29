#ifndef _write_SHAR_TALK_AUDIO_H
#define _write_SHAR_TALK_AUDIO_H
#include "StreDataType.h"
#include "audioType.h"
#include "shar_http.h"
#include "AudioDenoiser.h"
#include <algorithm> // 包含 std::find

#define BUFF_SIZE 1024

class SharTalkAudio
{
public:
    SharTalkAudio(std::string baseUrl);
    ~SharTalkAudio();

    bool sharInit(std::string sim, uint8_t loadType);
    bool write_shar_device(uint8_t *data, uint16_t size);
    void reint();
private:
    int ADPCM_decode(uint8_t *data, uint16_t size);
    int audio_decoder(uint8_t *data, uint16_t size);
    bool push_to_device(int shortPcmSize, audioType& audioInfo);
    bool write_data(audioType& audioInfo, uint16_t BodyLen);
    void add_map(const std::string& sim, const std::string& groupId);
    void alter_map(audioType& audioInfo);
    uint64_t get_timestamp();
    bool isSpeechPresent(const short* pcm, int sampleCount, int threshold = 500);
    
private:
    uint8_t  audio_type;

    uint8_t* ucOutBuff;
    adpcm_state* deState;

    //write 
    adpcm_state* enState;
    uint8_t* audioEncodeBuf;

    sharHttpSer* sharHttSer;
    std::string currentSIM;
    std::string groupID;

    AudioDenoiser *pAudioDenoiser;
};

extern bool get_audio_type_info(std::string sim,audioType& audioInfo);
extern void del_audio_type_info(std::string sim);

#endif
