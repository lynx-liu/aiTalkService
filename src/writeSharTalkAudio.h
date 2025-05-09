#ifndef _write_SHAR_TALK_AUDIO_H
#define _write_SHAR_TALK_AUDIO_H
#include "StreDataType.h"
#include "audioType.h"
#include "AAC2PCM.h"
#include "shar_adpcmstatemempool.h"
#include "shar_http.h"
#include <algorithm> // 包含 std::find


#define UC_OUT_BUFF_SIZE 1024
#define WR_OUT_BUFF_SIZE 1024
#define GET_STATUS_SIRST 0
#define GET_STATUS_SECOND 1
#define DECOND_STATUS_NO 0
#define DECOND_STATUS_YES 1
#define TIME_STATUS_START 0
#define TIME_STATUS_END 1

enum TIME_STAMP_STATUS_S
{
	TIME_STAMP_STATUS_OFF_S = 0,
	TIME_STAMP_STATUS_ON_S = 1
};

typedef unsigned char _BYTE; 
class SharTalkAudio
{
public:
    SharTalkAudio(/* args */);
    ~SharTalkAudio();

    bool sharInit(std::string sim, SEND_VIDEO_INFO_STRU* infoPtr, _BYTE loadType);
    bool write_shar_device(/*std::string sim, SEND_VIDEO_INFO_STRU* infoPtr, _BYTE loadType*/);  /*, SHAR_TALK_DATA_TYPE* PackHeDaInfo*/
    void reint();
private:
    void start_init();
    // void pushToDevice(std::string sim, SEND_VIDEO_INFO_STRU* infoPtr, _BYTE loadType, SHAR_TALK_DATA_TYPE* PackHeDaInfo);
    bool get_timestamp();
    bool G711A_decode();
    bool ADPCM_decode();
    bool audio_decoder();
    bool push_to_device_SIM10();
    bool push_to_device_SIM6();
    bool write_data_SIM10();
    bool write_data_SIM6();
    void add_map();
    void alter_map(std::string sim);
    unsigned long get_timestamp_S();
    void close_wrirteFd();
    bool audio_DeEncoder_judge();
    bool audio_Load_equal();
    bool audio_Load_inequality();
    bool audio_NoDePack_SIM10();
    bool audio_NoDePack_SIM6();
    // void get_sharDevice();
    bool Get_deviceIdMAP();
    void add_workmap();
    bool first_getDeviceInfo();
    bool device_status_info();
    void retur_appcmState_mempoolPtr();
    bool isSpeechPresent(const short* pcm, int sampleCount, int threshold = 500);
private:
    std::string    strID;
    std::map<std::string,  audioType> sharObjInfoMap;
    std::map<std::string,  audioType>::iterator iter;
    std::map<std::string,  audioType>::iterator _iter;
    std::map<std::string,  audioType> sharObjInfoMapTmp; 
    std::map<std::string, std::string> udeviInfomap;
    std::list<std::string> ulist;
    _BYTE       BCDSIMCardNumber[16];
    _BYTE       writePackHead[64];

    FRAM_HEADER* FramHeadPack;
    int          FramHeadPackLen;
    int          fd;

    TIME_STAMP_STATUS_S timeStampStatus;
    struct timeval tv;
	unsigned long int timestamp;

    SEND_VIDEO_INFO_STRU* dataInfoPtr;
    _BYTE* dataPtr;
    _BYTE  audio_type;
    _BYTE  Load_Type;

    _BYTE* ucOutBuff;
    _BYTE* wrLoadBuff;
    int    ucOutbuffSize;
    adpcm_state* state;
    adpcm_state* stateT;
    int    len;
    int count;

    //write 
    audioType audioTypeInfo;
    adpcm_state* state2;
    AUDIO_HEADER* auRtpPtr;
    AUDIO_HEADER_S* auRtpPtrS;

    _BYTE* wrOutBuff;
    unsigned short BodyLen;
    unsigned short num;
    unsigned HeaLeng;
    int iRet;
    ssize_t wriRet;
    int coun;

    unsigned HeaLengS;
    std::string sims;

    int     gstatus;
    int     deStatus;
    sharHttpSer* sharHttSer;
    time_t start_t, end_t;
    int    timestatu;
    std::string _sim;
    adpcmState* adpcmStaPtr;
};

extern bool get_audio_type_info2(std::map<std::string,  audioType>& _sharType);
extern void del_audio_type_info(std::string sim);

extern void install_deviceID(std::string sim, std::string strID);
extern void get_allDeviceID(std::map<std::string, std::string>& deviInfoMap);
extern void delete_deviceID_info(std::string sim);









#endif
