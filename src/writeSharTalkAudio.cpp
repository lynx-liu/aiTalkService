#include <cstddef>
#include "writeSharTalkAudio.h"

static std::string mainSIM = std::string();
static std::map<std::string, std::map<std::string,  audioType>> sharObjInfoMap;

SharTalkAudio::SharTalkAudio(std::string baseUrl)
{
    pAudioDenoiser = new AudioDenoiser(8000);

    ucOutBuff = new uint8_t[BUFF_SIZE]();
    deState = new adpcm_state();

    audioEncodeBuf = new uint8_t[BUFF_SIZE]();
    enState = new adpcm_state();
    auRtpPtr = new AUDIO_HEADER();

    sharHttSer= new sharHttpSer(baseUrl);
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
    if(pAudioDenoiser){
        delete pAudioDenoiser;
        pAudioDenoiser = nullptr;
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

    if (!currentSIM.empty()) {
        auto groupIt = sharObjInfoMap.find(groupID);
        if (groupIt != sharObjInfoMap.end()) {
            auto& simMap = groupIt->second;
    
            simMap.erase(currentSIM);
            
            if (mainSIM == currentSIM) {
                if (!simMap.empty()) {
                    mainSIM = simMap.begin()->first;
                } else {
                    mainSIM.clear();  // 当前组为空了
                }
            }
            
            if (simMap.empty()) {
                sharObjInfoMap.erase(groupIt);
            }
        }
    
        sharHttSer->POST_update(currentSIM, 4);
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

    groupID.clear();
    if(sharHttSer->POST_request(sim, groupID)){   //HTTP POST请求
        if(!groupID.empty()){
            printf("groupID: %s\n", groupID.data()); 
            add_map(currentSIM, groupID);
        }
        sharHttSer->POST_update(sim, 1);  //1.链接成功 2.离线  3. 正在进行 4.结束
    }
    return true;
}

void SharTalkAudio::add_map(const std::string& sim, const std::string& groupId)
{
    auto& simMap = sharObjInfoMap[groupId];
    if (simMap.find(sim) != simMap.end())
        return;

    audioType audioInfo;
    memset(&audioInfo, 0, sizeof(audioType));
    get_audio_type_info(sim, audioInfo);
    audioInfo.Bt8timeStamp = get_timestamp();

    simMap[sim] = audioInfo;
    printf("group size: %d, currentGroup size: %d\n", sharObjInfoMap.size(), simMap.size());
}

void SharTalkAudio::alter_map(audioType& audioInfo)
{
    audioInfo.Bt8timeStamp += 3;
    audioInfo.num += 1;
    audioInfo.index = enState->index;
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
        printf("isSpeechPresent: sim: %s, mainSIM: %s\n", currentSIM.c_str(), mainSIM.c_str());
        if(currentSIM != mainSIM) {
            mainSIM = currentSIM;
            memset(deState, 0, sizeof(adpcm_state));
            memset(enState, 0, sizeof(adpcm_state));
        }
    }
    
    if(currentSIM != mainSIM || groupID.empty()){
        return false;
    }

    auto groupIt = sharObjInfoMap.find(groupID);
    if (groupIt == sharObjInfoMap.end()) {
        printf("error groupId: %s\n", groupID.c_str());
        return false;
    }

    pAudioDenoiser->denoiseBuffer((short*)ucOutBuff, ucOutbuffSize/sizeof(short));

    auto& simMap = groupIt->second;
    //printf("mainSIM:%s, SIM: %s, size: %d\n", mainSIM.c_str(), currentSIM.c_str(), simMap.size());
    
    for (auto& simPair : simMap) {
        const std::string& sim = simPair.first;
        audioType& audioInfo = simPair.second;

        if (sim == currentSIM) continue;

        push_to_device(audioInfo);
        alter_map(audioInfo); 
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
		g711a_encode(audioEncodeBuf, (short*)ucOutBuff, ucOutbuffSize/sizeof(short));
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
        adpcm_decoder((char*)(dataPtr +8), (short*)ucOutBuff, len - 8, deState);
        ucOutbuffSize = (len - 8) * 4;
        return true;
    }

    deState->valprev = (short)(((dataPtr[1] << 8) & 0xff) | (dataPtr[0] & 0xff));
    deState->index = dataPtr[2];
    adpcm_decoder((char*)(dataPtr +4), (short*)ucOutBuff, len - 4, deState);
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
