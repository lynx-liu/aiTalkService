#include <unistd.h>
#include <cstddef>
#include <cstring>
#include <thread>
#include "writeSharTalkAudio.h"

#define WAIT_MAIN_SIM_TIME  2000 //ms
#define PCM_FRAME_SIZE  640

struct GroupAudioInfo {
    std::string mainSIM;
    uint64_t timestamp;
    std::map<std::string, audioType> simMap;
};

static std::map<std::string, GroupAudioInfo> sharObjInfoMap;

SharTalkAudio::SharTalkAudio(const CONFIG ServerConfig)
{
    pAudioDenoiser = new AudioDenoiser(8000);

    ucOutBuff = new uint8_t[BUFF_SIZE]();
    deState = new adpcm_state();

    audioEncodeBuf = new uint8_t[BUFF_SIZE]();
    enState = new adpcm_state();

    sharHttSer= new sharHttpSer(ServerConfig);
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
    isSpeaking = false;
    offset = 0;
    recvPcm.clear();
    pcmBuf.clear();

    if(ucOutBuff) memset(ucOutBuff, 0, BUFF_SIZE);
    if(audioEncodeBuf) memset(audioEncodeBuf, 0, BUFF_SIZE);
    if(deState) memset(deState, 0, sizeof(adpcm_state));
    if(enState) memset(enState, 0, sizeof(adpcm_state));

    if (!currentSIM.empty()) {
        auto groupIt = sharObjInfoMap.find(groupID);
        if (groupIt != sharObjInfoMap.end()) {
            GroupAudioInfo& groupInfo = groupIt->second;
            auto& simMap = groupInfo.simMap;

            simMap.erase(currentSIM);

            if (groupInfo.mainSIM == currentSIM) {
                if (!simMap.empty()) {
                    groupInfo.mainSIM = simMap.begin()->first;
                    groupInfo.timestamp = get_timestamp();
                    sharHttSer->POST_update(groupInfo.mainSIM, 5);  //1.链接成功 2.离线  3. 正在进行 4.结束, 5. 成为主讲人
                } else {
                    groupInfo.mainSIM.clear();  // 当前组为空了
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

bool SharTalkAudio::sharInit(std::string sim, uint8_t loadType)
{
    currentSIM = sim;
    audio_type = loadType&0x7F;

    isSpeaking = false;
    offset = 0;
    recvPcm.clear();
    pcmBuf.clear();

    groupID.clear();
    if(sharHttSer->POST_request(sim, groupID)){   //HTTP POST请求
        if(!groupID.empty()){
            printf("groupID: %s\n", groupID.data()); 
            add_map(currentSIM, groupID);
            return sharHttSer->POST_update(sim, 1);  //1.链接成功 2.离线  3. 正在进行 4.结束, 5. 成为主讲人
        }
    }
    return false;
}

void SharTalkAudio::add_map(const std::string& sim, const std::string& groupId)
{
    GroupAudioInfo& groupInfo = sharObjInfoMap[groupId];
    auto& simMap = groupInfo.simMap;

    if (simMap.find(sim) != simMap.end())
        return;

    audioType audioInfo;
    memset(&audioInfo, 0, sizeof(audioType));
    get_audio_type_info(sim, audioInfo);
    audioInfo.Bt8timeStamp = get_timestamp();
    simMap[sim] = audioInfo;

    if (groupInfo.mainSIM.empty()) {
        groupInfo.mainSIM = sim;
        groupInfo.timestamp = get_timestamp();
        sharHttSer->POST_update(groupInfo.mainSIM, 5);  //1.链接成功 2.离线  3. 正在进行 4.结束, 5. 成为主讲人
    }

    printf("group size: %zu, currentGroup size: %zu\n", sharObjInfoMap.size(), simMap.size());
}

void SharTalkAudio::alter_map(audioType& audioInfo)
{
    audioInfo.Bt8timeStamp += 3;
    audioInfo.num += 1;
}


uint64_t SharTalkAudio::get_timestamp()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

bool SharTalkAudio::write_shar_device(uint8_t *data, uint16_t size)
{
    int shortPcmSize = audio_decoder(data, size)/sizeof(short);
    if (shortPcmSize<=0) {
        printf("audio_decoder fail!\n");
        return false;
    }

    auto groupIt = sharObjInfoMap.find(groupID);
    if (groupIt == sharObjInfoMap.end()) {
        return false;
    }

    GroupAudioInfo& groupInfo = groupIt->second;
    auto& simMap = groupInfo.simMap;

    int64_t currentTime = get_timestamp();
    if(isSpeechPresent((short*)ucOutBuff, shortPcmSize)) {
        printf("isSpeechPresent: sim: %s, mainSIM: %s\n", currentSIM.c_str(), groupInfo.mainSIM.c_str());
        if(!isSpeaking) {
            isSpeaking = true;
            printf("%s isSpeaking = true\n", currentSIM.c_str());

            if(currentSIM==groupID) {//currentSIM与groupID相同时,表示AI对讲虚拟的groupID
                pcmBuf.clear();
            }
        }

        if(currentSIM != groupInfo.mainSIM) {
            if(currentTime-groupInfo.timestamp>WAIT_MAIN_SIM_TIME) {
                groupInfo.mainSIM = currentSIM;
                groupInfo.timestamp = currentTime;
                std::thread([this, groupInfo](){
                    sharHttSer->POST_update(groupInfo.mainSIM, 5);  //1.链接成功 2.离线  3. 正在进行 4.结束, 5. 成为主讲人
                }).detach();

                memset(deState, 0, sizeof(adpcm_state));
                memset(enState, 0, sizeof(adpcm_state));
            }
        } else {
            groupInfo.timestamp = currentTime;
        }
    } else if(isSpeaking && currentTime-groupInfo.timestamp>WAIT_MAIN_SIM_TIME){
        isSpeaking = false;
        printf("%s isSpeaking = false\n", currentSIM.c_str());

        if(currentSIM==groupID) {//currentSIM与groupID相同时,表示AI对讲虚拟的groupID
            std::thread([this](){
                offset = 0;
                recvPcm = sharHttSer->POST_pcm(currentSIM, pcmBuf);
                printf("recv pcm size: %zu\n", recvPcm.size());
            }).detach();
        }
    }
    
    if(currentSIM != groupInfo.mainSIM || groupID.empty()){
        return false;
    }

    pAudioDenoiser->denoiseBuffer((short*)ucOutBuff, shortPcmSize);
    //printf("mainSIM:%s, SIM: %s, size: %d\n", groupInfo.mainSIM.c_str(), currentSIM.c_str(), simMap.size());
    
    for (auto& simPair : simMap) {
        const std::string& sim = simPair.first;
        audioType& audioInfo = simPair.second;

        if (sim == currentSIM) {
            if(currentSIM==groupID) {//currentSIM与groupID相同时,表示AI对讲虚拟的groupID
                if(isSpeaking) {
                    appendPCMData(ucOutBuff, shortPcmSize*sizeof(short));
                    return true;
                } else {
                    if(recvPcm.empty())
                        return true;

                    if(offset+PCM_FRAME_SIZE<=recvPcm.size()) {
                        if(offset==0) {
                            memset(deState, 0, sizeof(adpcm_state));
                            memset(enState, 0, sizeof(adpcm_state));
                        }

                        memcpy(ucOutBuff, &recvPcm[offset], PCM_FRAME_SIZE);
                        offset += PCM_FRAME_SIZE;
                        shortPcmSize = PCM_FRAME_SIZE/sizeof(short);
                    } else {
                        offset = 0;
                        recvPcm.clear();
                        printf("ai talk finish\n");
                        continue;
                    }
                }
            } else {
                continue;
            }
        }

        push_to_device(shortPcmSize, audioInfo);
        alter_map(audioInfo); 
    }
    return true;
}

void SharTalkAudio::appendPCMData(const uint8_t* pcm, size_t size) {
    pcmBuf.reserve(pcmBuf.size() + size);
    pcmBuf.insert(pcmBuf.end(), pcm, pcm + size);
}

bool SharTalkAudio::push_to_device(int shortPcmSize, audioType& audioInfo)
{
    uint16_t BodyLen = 0;
	memset(audioEncodeBuf, 0, BUFF_SIZE);
	if((audioInfo.type&0x7F) == LOAD_TYPE_G711A){
		g711a_encode(audioEncodeBuf, (short*)ucOutBuff, shortPcmSize);
		BodyLen = shortPcmSize;
	}else if((audioInfo.type&0x7F) == LOAD_TYPE_ADPCM){
		memcpy(audioEncodeBuf,audioInfo.ADPCM_8, 4);
		audioEncodeBuf[4] = (enState->valprev & 0xff);
		audioEncodeBuf[5] = ((enState->valprev >> 8) & 0xff);
		audioEncodeBuf[6] = enState->index;
		audioEncodeBuf[7] = 0x00;
		adpcm_coder((short*)ucOutBuff, (char*)(audioEncodeBuf+8), shortPcmSize, enState);
		BodyLen = (shortPcmSize / 2) + 8;
	}else{
        return false;
    }
	return write_data(audioInfo, BodyLen);
}

bool SharTalkAudio::write_data(audioType& audioInfo, uint16_t BodyLen)
{
    RTP_PKG_HEADER pkg;
	pkg.type  = audioInfo.type;              
	memcpy(pkg.BCDSIMCardNumber, audioInfo.BCDSIMCardNumber, audioInfo.BCDSIMLen);
	pkg.Bt1LogicChannelNumber = audioInfo.ChannelNumber;
	pkg.WdBodyLen = htons(BodyLen);
    pkg.WdPackageSequence = htons(audioInfo.num);
	pkg.Bt8timeStamp = htonll(audioInfo.Bt8timeStamp); 
    pkg.DWFramHeadMark = htonl(pkg.DWFramHeadMark);

	if(!(write(audioInfo.socketFd, &pkg, offsetof(RTP_PKG_HEADER, BCDSIMCardNumber)+audioInfo.BCDSIMLen) &&
        write(audioInfo.socketFd, (uint8_t*)&pkg + offsetof(RTP_PKG_HEADER, Bt1LogicChannelNumber), sizeof(RTP_PKG_HEADER) - offsetof(RTP_PKG_HEADER, Bt1LogicChannelNumber))))
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

int SharTalkAudio::audio_decoder(uint8_t *data, uint16_t size)
{
    if(!ucOutBuff) return 0;
    memset(ucOutBuff, 0, BUFF_SIZE);

    if(audio_type == LOAD_TYPE_G711A){
        return g711a_decode((short*)ucOutBuff, data, size);
    }else if(audio_type == LOAD_TYPE_ADPCM){ 
        return ADPCM_decode(data, size);
    }
    return 0;
}

int SharTalkAudio::ADPCM_decode(uint8_t *data, uint16_t size)
{
    if (data[0] == 0x00 && data[1] == 0x01 && (data[2] & 0xff) == (size - 4) / 2 && data[3] == 0x00){
        deState->valprev = (short)(((data[5] << 8) & 0xff) | (data[4] & 0xff));
        deState->index = data[6];
        adpcm_decoder((char*)(data +8), (short*)ucOutBuff, size - 8, deState);
        return (size - 8) * 4;
    }

    deState->valprev = (short)(((data[1] << 8) & 0xff) | (data[0] & 0xff));
    deState->index = data[2];
    adpcm_decoder((char*)(data +4), (short*)ucOutBuff, size - 4, deState);
    return (size - 4) * 4;
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
    //printf("sim: %s, avg: %d, count: %d sum:%lld\n", currentSIM.c_str(), avgAmplitude, sampleCount, sumAbs);
    return avgAmplitude > threshold;
}
