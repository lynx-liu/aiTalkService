#include "writeSharTalkAudio.h"

static std::string mainSIM = std::string();
static std::map<std::string,  audioType> sharObjInfoMap;

SharTalkAudio::SharTalkAudio(/* args */)
{
    ucOutBuff = new uint8_t[BUFF_SIZE]();
    deState = new adpcm_state();

    audioEncodeBuf = new uint8_t[BUFF_SIZE]();
    enState = new adpcm_state();
    auRtpPtr = new AUDIO_HEADER();

    sharHttSer= new sharHttpSer();
    currentSIM.clear();
}

SharTalkAudio::~SharTalkAudio()
{
    if(ucOutBuff){
        delete [] ucOutBuff;
        ucOutBuff = nullptr;
    }
    if (deState){
        delete deState;
        deState = nullptr;
    }
    if(audioEncodeBuf){
		delete [] audioEncodeBuf;
		audioEncodeBuf = nullptr;
	}
    if(enState){
		delete enState;
		enState = nullptr;
	}
    if(auRtpPtr){
        delete auRtpPtr;
        auRtpPtr = nullptr;
    }
    if(sharHttSer){
        delete sharHttSer;
        sharHttSer = nullptr;
    }
}

void SharTalkAudio::reint()
{
    ucOutbuffSize = 0;
    if(ucOutBuff) memset(ucOutBuff, 0, BUFF_SIZE);
    if(audioEncodeBuf) memset(audioEncodeBuf, 0, BUFF_SIZE);
    if(deState) memset(deState, 0, sizeof(adpcm_state));
    if(enState) memset(enState, 0, sizeof(adpcm_state));
    if(auRtpPtr) memset(auRtpPtr, 0, sizeof(AUDIO_HEADER));

    if(!currentSIM.empty()){
        sharObjInfoMap.erase(currentSIM);
        delete_deviceID_info(currentSIM);
        sharHttSer->POST_update(currentSIM, 4);

        if(mainSIM==currentSIM) {
            mainSIM = sharObjInfoMap .begin()->first;
        }
        currentSIM.clear();
    }
}

bool SharTalkAudio::sharInit(std::string sim, SEND_VIDEO_INFO_STRU* infoPtr, uint8_t loadType)
{
    currentSIM = sim;
    dataInfoPtr = infoPtr;
    audio_type = (loadType & 0xFF);

    if(mainSIM.empty()){
        mainSIM = currentSIM;
    }

    std::string strID;
    if(sharHttSer->POST_request(sim, strID)){   //HTTP POST请求
        if(!strID.empty()){
            install_deviceID(sim, strID);
            printf("id: %s\n", strID.data());  
        }
        sharHttSer->POST_update(sim, 1);  //1.链接成功 2.离线  3. 正在进行 4.结束
    }

    add_map();
    return true;
}

void SharTalkAudio::add_map()
{
    std::map<std::string,  audioType> sharObjInfoMapTmp; 
    get_audio_type_info2(sharObjInfoMapTmp);
    
    for (const auto& pair : sharObjInfoMapTmp){
        std::map<std::string,  audioType>::iterator iter = sharObjInfoMap.find(pair.first);
        if (iter == sharObjInfoMap.end()){
            audioType audioInfo;
            memset(&audioInfo, 0, sizeof(audioType));
            audioInfo = pair.second;
            audioInfo.Bt8timeStamp = get_timestamp();
            sharObjInfoMap[pair.first] = audioInfo;
        }
    } 
}

void SharTalkAudio::alter_map(std::string sim)
{
    audioType audioInfo;
    memset(&audioInfo, 0, sizeof(audioType));
    std::map<std::string,  audioType>::iterator iter = sharObjInfoMap.find(sim);
    if(iter != sharObjInfoMap.end()){
        audioInfo = iter->second;
        audioInfo.Bt8timeStamp += 3;
        audioInfo.num += 1;
        audioInfo.index = enState->index;  
        iter->second  = audioInfo;
    }
}

uint64_t SharTalkAudio::get_timestamp()
{
    struct timeval tv;
	uint64_t timestamp;
    gettimeofday(&tv, NULL);
	timestamp = (tv.tv_sec * 1000 + tv.tv_usec / 1000);
    return timestamp;
}

bool SharTalkAudio::write_shar_device()
{
    if (!audio_decoder()) {
        printf("audio_type fail!\n");
        return false;
    }

    if(isSpeechPresent((short*)ucOutBuff, ucOutbuffSize/sizeof(short))) {
        printf("isSpeechPresent: sim: %s, mainSIM: %s, size:%d\n", currentSIM.c_str(), mainSIM.c_str(), sharObjInfoMap.size());
        if(currentSIM != mainSIM) {
            mainSIM = currentSIM;
            memset(deState, 0, sizeof(adpcm_state));
            memset(enState, 0, sizeof(adpcm_state));
        }
    }
    
    //printf("mainSIM:%s, SIM: %s, size: %d\n", mainSIM.c_str(), currentSIM.c_str(), sharObjInfoMap.size());
    if(currentSIM != mainSIM){
        return false;
    }

    for (const auto& pair : sharObjInfoMap) {
        if(pair.first == currentSIM) continue;
        push_to_device(pair.second);
        alter_map(pair.first);
    }
}

bool SharTalkAudio::push_to_device(audioType audioInfo)
{
	if(!auRtpPtr) return false;
	 
	auRtpPtr->info1          = 0x81;
	auRtpPtr->info2          = audioInfo.Tag_PayloadType;              
	memcpy(auRtpPtr->BCDSIMCardNumber, audioInfo.BCDSIMCardNumber, audioInfo.BCDSIMLen);
	auRtpPtr->Bt1LogicChannelNumber = audioInfo.ChannelNumber;
	auRtpPtr->info3          = 0x30;

    BodyLen = 0;
	memset(audioEncodeBuf, 0, BUFF_SIZE);
	if(audioInfo.Tag_PayloadType == 0x86){
		g711a_encode(audioEncodeBuf, (short*)ucOutBuff, ucOutbuffSize);
		BodyLen = (uint16_t)(ucOutbuffSize / 2);
	}else if(audioInfo.Tag_PayloadType == 0x9A){
		memcpy(audioEncodeBuf,audioInfo.ADPCM_8, 4);
		audioEncodeBuf[4] = (enState->valprev & 0xff);
		audioEncodeBuf[5] = ((enState->valprev >> 8) & 0xff);
		audioEncodeBuf[6] = enState->index;
		audioEncodeBuf[7] = 0x00;
		adpcm_coder((short*)ucOutBuff, (char*)(audioEncodeBuf+8), ucOutbuffSize / 2, enState);
		BodyLen = (ucOutbuffSize / 4) + 8;
	}else{
        return false;
    }

	auRtpPtr->WdBodyLen = htons(BodyLen);
    auRtpPtr->WdPackageSequence = htons(audioInfo.num);
	get_timestamp();

	auRtpPtr->Bt8timeStamp = htonl(audioInfo.Bt8timeStamp); 
	return write_data(audioInfo);
}

bool SharTalkAudio::write_data(audioType& audioInfo)
{
	if(!(write(audioInfo.socketFd, auRtpPtr, offsetof(AUDIO_HEADER, BCDSIMCardNumber)+audioInfo.BCDSIMLen) &&
        write(audioInfo.socketFd, (uint8_t*)auRtpPtr + offsetof(AUDIO_HEADER, Bt1LogicChannelNumber), sizeof(AUDIO_HEADER) - offsetof(AUDIO_HEADER, Bt1LogicChannelNumber))))
    {
        close(audioInfo.socketFd);
		perror("errno:");
		return false;
	}
 
	int count = 0;
	do{
        int size = write(audioInfo.socketFd, audioEncodeBuf+count, BodyLen-count);
		if(size < 0){
            close(audioInfo.socketFd);
			perror("errno:");
			return false;
		}
		count += size;
	}while ((0 < count) && (count<BodyLen));

	return true;
}

bool SharTalkAudio::audio_decoder()
{
    if(!dataInfoPtr) return false;
    if(!ucOutBuff) return false;

    ucOutbuffSize = 0;
    memset(ucOutBuff, 0, BUFF_SIZE);
    if(audio_type == LOAD_TYPE_G711A_TYPE){
        if(!G711A_decode()) return false;
    }else if(audio_type == LOAD_TYPE_ADPCM_TYPE){ 
        if(!ADPCM_decode()) return false;
    }else{
        return false;
    }

    return true;
}


bool SharTalkAudio::G711A_decode()
{
    if(!dataInfoPtr || !dataInfoPtr->VidePacData) return false;

    ucOutbuffSize = g711a_decode((short*)ucOutBuff, dataInfoPtr->VidePacData, dataInfoPtr->WdBodyLen);
    return true;
}

bool SharTalkAudio::ADPCM_decode()
{

    if(!deState || !dataInfoPtr) return false;

    uint8_t* dataPtr = dataInfoPtr->VidePacData;
    int len = dataInfoPtr->WdBodyLen;

    if (dataPtr[0] == 0x00 && dataPtr[1] == 0x01 && (dataPtr[2] & 0xff) == (len - 4) / 2 && dataPtr[3] == 0x00){
        deState->valprev = (short)(((dataPtr[5] << 8) & 0xff) | (dataPtr[4] & 0xff));
        deState->index = dataPtr[6];
        adpcm_decoder((char*)(dataPtr +8), (short*)ucOutBuff, (len - 8) * 2, deState);
        ucOutbuffSize = (len - 8) * 4;
        return true;
    }

    deState->valprev = (short)(((dataPtr[1] << 8) & 0xff) | (dataPtr[0] & 0xff));
    deState->index = dataPtr[2];
    adpcm_decoder((char*)(dataPtr +4), (short*)ucOutBuff, (len - 4) * 2, deState);
    ucOutbuffSize = (len - 4) * 4;
   
    return true;
}

bool SharTalkAudio::isSpeechPresent(const short* pcm, int sampleCount, int threshold)
{
    if (!pcm || sampleCount <= 0) return false;

    long long sumAbs = 0;
    for (int i = 0; i < sampleCount; i++) {
        short power = std::abs(pcm[i]);
        sumAbs += power;
    }

    int avgAmplitude = sumAbs / sampleCount;
    //printf("\nsim: %s, avg: %d, count: %d sum:%lld\n", currentSIM.c_str(), avgAmplitude, sampleCount, sumAbs);
    return avgAmplitude > threshold;
}
