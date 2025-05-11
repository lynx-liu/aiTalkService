#ifndef _write_SHAR_TALK_AUDIO_H
#define _write_SHAR_TALK_AUDIO_H
#include "StreDataType.h"
#include "audioType.h"
#include "AAC2PCM.h"
#include "shar_http.h"
#include <algorithm> // 包含 std::find

#define BUFF_SIZE 1024

class SharTalkAudio
{
public:
    SharTalkAudio(/* args */);
    ~SharTalkAudio();

    bool sharInit(std::string sim, SEND_VIDEO_INFO_STRU* infoPtr, uint8_t loadType);
    bool write_shar_device();
    void reint();
private:
    bool G711A_decode();
    bool ADPCM_decode();
    bool audio_decoder();
    bool push_to_device(audioType audioInfo);
    bool write_data(audioType& audioInfo);
    void add_map();
    void alter_map(std::string sim);
    uint64_t get_timestamp();
    bool isSpeechPresent(const short* pcm, int sampleCount, int threshold = 500);
    
private:
    SEND_VIDEO_INFO_STRU* dataInfoPtr;
    uint8_t  audio_type;

    uint8_t* ucOutBuff;
    int    ucOutbuffSize;
    adpcm_state* deState;

    //write 
    adpcm_state* enState;
    AUDIO_HEADER* auRtpPtr;

    uint8_t* audioEncodeBuf;
    uint16_t BodyLen;

    sharHttpSer* sharHttSer;
    std::string currentSIM;
};

extern bool get_audio_type_info2(std::map<std::string,  audioType>& _sharType);
extern void del_audio_type_info(std::string sim);

extern void install_deviceID(std::string sim, std::string strID);
extern void delete_deviceID_info(std::string sim);

#endif
