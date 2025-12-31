#include <unistd.h>
#include <cstddef>
#include <cstring>
#include <thread>
#include "debug.h"
#include "tiny_ws.h"
#include "writeSharTalkAudio.h"

#define WAIT_MAIN_SIM_TIME  2000 //ms
#define PCM_FRAME_SIZE      640

#define TYPE_WS_WEB_TALK    0x00 //通过websocket平台对讲
#define TYPE_GROUP_TALK     0x01 //群组对讲
#define TYPE_AI_TALK        0x02 //AI对讲
#define TYPE_WS_VAR_TALK    0x04 //多变量通知触发的对讲

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
    wsRecvPcm.clear();
    httpRecvPcm.clear();
    pcmBuf.clear();
    webSocketFd = -1;
    playingStartTime = 0;

    responseHeader.ad_hear_record_id.clear();
    responseHeader.x_file_type.clear();

    if(ucOutBuff) memset(ucOutBuff, 0, BUFF_SIZE);
    if(audioEncodeBuf) memset(audioEncodeBuf, 0, BUFF_SIZE);
    if(deState) memset(deState, 0, sizeof(adpcm_state));
    if(enState) memset(enState, 0, sizeof(adpcm_state));

    if (!currentSIM.empty()) {
        tiny_ws::remove_callback(currentSIM);
        
        auto groupIt = sharObjInfoMap.find(groupID);
        if (groupIt != sharObjInfoMap.end()) {
            GroupAudioInfo& groupInfo = groupIt->second;
            auto& simMap = groupInfo.simMap;

            simMap.erase(currentSIM);

            if (groupInfo.mainSIM == currentSIM) {
                if (!simMap.empty()) {
                    groupInfo.mainSIM = simMap.begin()->first;
                    groupInfo.timestamp = get_timestamp();
                    sharHttSer->updateTalkingState(groupInfo.mainSIM, 5);  //1.链接成功 2.离线  3. 正在进行 4.结束, 5. 成为主讲人
                } else {
                    groupInfo.mainSIM.clear();  // 当前组为空了
                }
            }
            
            if (simMap.empty()) {
                sharObjInfoMap.erase(groupIt);
            }
        }
    
        if(type&TYPE_GROUP_TALK) {
            sharHttSer->updateTalkingState(currentSIM, 4);
        }
        currentSIM.clear();
    }
}

bool SharTalkAudio::sharInit(std::string sim, uint8_t loadType)
{
    currentSIM = sim;
    audio_type = loadType&0x7F;

    isSpeaking = false;
    offset = 0;
    wsRecvPcm.clear();
    httpRecvPcm.clear();
    pcmBuf.clear();

    groupID.clear();
    type = 0;
    webSocketFd = -1;
    playingStartTime = 0;

    responseHeader.ad_hear_record_id.clear();
    responseHeader.x_file_type.clear();

    if(sharHttSer->getTalkingInfo(currentSIM, groupID, type)){   //HTTP POST请求
        if(!groupID.empty()){
            add_map(currentSIM, groupID);

            webSocketFd=tiny_ws::get_client_fd(currentSIM);
            printf("\n%sgroupID %s, webSocketFd: %d, type: %d", getNowTime().data(), groupID.data(), webSocketFd, type);
            if(webSocketFd < 0) {// 不是平台对讲
                if(!sharHttSer->updateTalkingState(sim, 1))  //1.链接成功 2.离线  3. 正在进行 4.结束, 5. 成为主讲人
                    return false;
            }
  
            auto weak_this = std::weak_ptr<SharTalkAudio>(
                std::static_pointer_cast<SharTalkAudio>(shared_from_this())
            );

            // 平台对讲回调　或者ws未连接时预设回调
            tiny_ws::set_callback(currentSIM, [weak_this](const std::vector<uint8_t>& data) {
                (data.size()==PCM_FRAME_SIZE)? printf("-") : printf("%zu\n",data.size());

                if (auto self = weak_this.lock()) {
                    std::lock_guard<std::mutex> lock(self->pcm_mutex);
                    if (self->wsRecvPcm.size() < PCM_FRAME_SIZE*1024) {
                        self->wsRecvPcm.insert(self->wsRecvPcm.end(), data.begin(), data.end());
                    }
                }
            }, [weak_this](int type) {
                if (auto self = weak_this.lock()) {
                    std::lock_guard<std::mutex> lock(self->pcm_mutex);
                    self->type |= type;
                    if(self->webSocketFd<0) {
                        self->webSocketFd = tiny_ws::get_client_fd(self->currentSIM);
                    }

                    memset(self->deState, 0, sizeof(adpcm_state));
                    memset(self->enState, 0, sizeof(adpcm_state));
                    printf("\n%sReceived type change: %d --> %d , fd= %d", getNowTime().data(), type, self->type, self->webSocketFd);
                }
            }, [weak_this](int type) {
                if (auto self = weak_this.lock()) {
                    std::lock_guard<std::mutex> lock(self->pcm_mutex);
                    self->webSocketFd = -1;
                    if(type>0) {// 清除对应的type标志
                        self->type &= ~type;
                    }
                    printf("\n%sws disconnected, type: %d --> %d", getNowTime().data(), type, self->type);

                    // 从sharObjInfoMap中移除currentSIM
                    auto groupIt = sharObjInfoMap.find(self->groupID);
                    if (groupIt != sharObjInfoMap.end()) {
                        GroupAudioInfo& groupInfo = groupIt->second;
                        auto& simMap = groupInfo.simMap;

                        simMap.erase(self->currentSIM);
                        printf("\n%sremove SIM: %s from groupID: %s", getNowTime().data(), self->currentSIM.data(), self->groupID.data());
                        
                        if (simMap.empty()) {
                            groupInfo.mainSIM.clear();  // 当前组为空了
                            sharObjInfoMap.erase(groupIt);
                            printf("\n%sremove groupID: %s", getNowTime().data(), self->groupID.data());
                        }
                    }

                    if(self->sharHttSer->getTalkingInfo(self->currentSIM, self->groupID, type)) { //重新获取对讲信息
                        self->type |= type;
                        
                        if(!self->groupID.empty()){
                            printf("\n%sgroupID: %s, type: %d --> %d", getNowTime().data(), self->groupID.data(), type, self->type);
                            self->add_map(self->currentSIM, self->groupID);
                        }
                    }

                    memset(self->deState, 0, sizeof(adpcm_state));
                    memset(self->enState, 0, sizeof(adpcm_state));
                }
            });
            return true;
        } else {
            printf("\n%ssharInit fail, groupID empty!", getNowTime().data());
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
        if(type&TYPE_GROUP_TALK) {
            sharHttSer->updateTalkingState(groupInfo.mainSIM, 5);  //1.链接成功 2.离线  3. 正在进行 4.结束, 5. 成为主讲人
        }
    }

    printf("\n%sgroup size: %zu, currentGroup size: %zu", getNowTime().data(), sharObjInfoMap.size(), simMap.size());
}

void SharTalkAudio::alter_map(audioType& audioInfo)
{
    audioInfo.Bt8timeStamp += 3;
    audioInfo.num += 1;
}

bool SharTalkAudio::wsplayback(audioType& audioInfo, const uint8_t* pcm, int shortPcmSize)
{
    tiny_ws::send_bin(webSocketFd, pcm, shortPcmSize * sizeof(short));

    uint8_t buf[PCM_FRAME_SIZE] = {0};
    int frameShortPcmSize = PCM_FRAME_SIZE / sizeof(short);

    while (wsRecvPcm.size() >= PCM_FRAME_SIZE) {
        memcpy(buf, wsRecvPcm.data(), PCM_FRAME_SIZE);
        wsRecvPcm.erase(wsRecvPcm.begin(), wsRecvPcm.begin() + PCM_FRAME_SIZE);

        push_to_device(buf, frameShortPcmSize, audioInfo);
        alter_map(audioInfo);
    }

    //处理剩余不足一帧的数据
    if (!wsRecvPcm.empty()) {
        size_t remainingSize = wsRecvPcm.size();
        memcpy(buf, wsRecvPcm.data(), remainingSize);
        memset(buf + remainingSize, 0, PCM_FRAME_SIZE - remainingSize); // 补零

        push_to_device(buf, frameShortPcmSize, audioInfo);
        alter_map(audioInfo);

        wsRecvPcm.clear();
    }
    return true;
}

bool SharTalkAudio::httpplayback(audioType& audioInfo, const uint8_t* pcm, int shortPcmSize)
{
    if(isSpeaking) {//司机在讲话，缓存pcm数据
        if(playingStartTime>0) {//广告播放被中断
            int64_t playingTime = get_timestamp()-playingStartTime;
            playingStartTime = 0;
            sharHttSer->updateVoiceState(responseHeader.ad_hear_record_id, "03"/*中断*/, playingTime);
            responseHeader.ad_hear_record_id.clear();
            responseHeader.x_file_type.clear();
        }

        appendPCMData(pcm, shortPcmSize*sizeof(short));
        return true;
    }
    
    if(httpRecvPcm.empty() || wsRecvPcm.size()>0)
        return true;//没有AI对讲数据，或者还有未处理多变量的ws数据

    if(offset+PCM_FRAME_SIZE<=httpRecvPcm.size()) {
        if(offset==0) {
            memset(deState, 0, sizeof(adpcm_state));
            memset(enState, 0, sizeof(adpcm_state));
        }

        memcpy(ucOutBuff, &httpRecvPcm[offset], PCM_FRAME_SIZE);
        offset += PCM_FRAME_SIZE;
        shortPcmSize = PCM_FRAME_SIZE/sizeof(short);

        push_to_device(ucOutBuff, shortPcmSize, audioInfo);
        alter_map(audioInfo);
    } else {
        offset = 0;
        httpRecvPcm.clear();
        printf("\n%sai talk finish", getNowTime().data());

        if(playingStartTime>0) {//广告
            int64_t playingTime = get_timestamp()-playingStartTime;
            playingStartTime = 0;
            sharHttSer->updateVoiceState(responseHeader.ad_hear_record_id, "02"/*完成*/, playingTime);
            responseHeader.ad_hear_record_id.clear();
            responseHeader.x_file_type.clear();
        }
    }
    return true;
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
        printf("\n%saudio_decoder fail!", getNowTime().data());
        return false;
    }

    auto groupIt = sharObjInfoMap.find(groupID);
    if (groupIt == sharObjInfoMap.end()) {
        return false;
    }

    GroupAudioInfo& groupInfo = groupIt->second;
    auto& simMap = groupInfo.simMap;

    int64_t currentTime = get_timestamp();
    if(isSpeechPresent((short*)ucOutBuff, shortPcmSize)) {//司机在讲话
        if(!isSpeaking) {
            isSpeaking = true;
            printf("\n%s%s isSpeaking = true", getNowTime().data(), currentSIM.c_str());

            if(type&TYPE_AI_TALK || type&TYPE_WS_VAR_TALK) {//AI对讲或多变量对讲
                pcmBuf.clear();
            }
        }

        if(currentSIM != groupInfo.mainSIM) {
            if(currentTime-groupInfo.timestamp>WAIT_MAIN_SIM_TIME) {
                groupInfo.mainSIM = currentSIM;
                groupInfo.timestamp = currentTime;
                std::thread([this, groupInfo](){
                    sharHttSer->updateTalkingState(groupInfo.mainSIM, 5);  //1.链接成功 2.离线  3. 正在进行 4.结束, 5. 成为主讲人
                }).detach();

                memset(deState, 0, sizeof(adpcm_state));
                memset(enState, 0, sizeof(adpcm_state));
            }
        } else {
            groupInfo.timestamp = currentTime;
        }
    } else if(isSpeaking && currentTime-groupInfo.timestamp>WAIT_MAIN_SIM_TIME){//司机停止讲话
        isSpeaking = false;
        printf("\n%s%s isSpeaking = false", getNowTime().data(), currentSIM.c_str());

        if(type&TYPE_AI_TALK || type&TYPE_WS_VAR_TALK) {//AI对讲或多变量对讲
            auto weak_this = std::weak_ptr<SharTalkAudio>(
                std::static_pointer_cast<SharTalkAudio>(shared_from_this())
            );
            
            std::thread([weak_this]() {
                if (auto self = weak_this.lock()) {
                    self->offset = 0;
                    self->httpRecvPcm = self->sharHttSer->POST_pcm(self->currentSIM, self->pcmBuf, self->responseHeader);
                    printf("\n%spush pcm size: %zu, recv pcm size: %zu, id: %s, type: %s", getNowTime().data(), self->pcmBuf.size(), self->httpRecvPcm.size(), self->responseHeader.ad_hear_record_id.c_str(), self->responseHeader.x_file_type.c_str());
                    if (self->responseHeader.x_file_type == "02") { // 广告
                        self->playingStartTime = self->get_timestamp();
                    }
                }
            }).detach();
        }
    }

    pAudioDenoiser->denoiseBuffer((short*)ucOutBuff, shortPcmSize);
    //printf("\n%smainSIM:%s, SIM: %s, size: %d", getNowTime().data(), groupInfo.mainSIM.c_str(), currentSIM.c_str(), simMap.size());
    
    std::lock_guard<std::mutex> lock(pcm_mutex);//确保webSocketFd和type线程同步更新，保护simMap遍历时不被修改，以及wsRecvPcm和httpRecvPcm数据一致性
    for (auto& simPair : simMap) {
        const std::string& sim = simPair.first;
        audioType& audioInfo = simPair.second;

        if (sim == currentSIM) {
            printf(" %d", type);
            if(webSocketFd>0) {//ws连上了，可能是多变量对讲，也可能是平台对讲
                wsplayback(audioInfo, ucOutBuff, shortPcmSize);
            }
            
            if(type&TYPE_WS_VAR_TALK || type&TYPE_AI_TALK) {//多变量对讲 或 AI对讲
                httpplayback(audioInfo, ucOutBuff, shortPcmSize);
            }
            continue;
        }

        if(currentSIM != groupInfo.mainSIM){
            continue;//不是主讲人，不发送对讲数据
        }

        if(type&TYPE_WS_VAR_TALK) {
            continue;//多变量对讲开启时禁用群组对讲
        }

        if(tiny_ws::get_client_fd(sim) > 0) {//主讲人不向已打开ws的用户发送对讲数据
            continue;
        }

        //群组对讲
        push_to_device(ucOutBuff, shortPcmSize, audioInfo);
        alter_map(audioInfo); 
    }
    return true;
}

void SharTalkAudio::appendPCMData(const uint8_t* pcm, size_t size) {
    pcmBuf.insert(pcmBuf.end(), pcm, pcm + size);
}

bool SharTalkAudio::push_to_device(const uint8_t* pcm, int shortPcmSize, audioType& audioInfo)
{
    uint16_t BodyLen = 0;
	memset(audioEncodeBuf, 0, BUFF_SIZE);
    if((audioInfo.type&0x7F) == LOAD_TYPE_G711A){
        g711a_encode(audioEncodeBuf, (short*)pcm, shortPcmSize);
		BodyLen = shortPcmSize;
	}else if((audioInfo.type&0x7F) == LOAD_TYPE_ADPCM){
		memcpy(audioEncodeBuf,audioInfo.ADPCM_8, 4);
		audioEncodeBuf[4] = (enState->valprev & 0xff);
		audioEncodeBuf[5] = ((enState->valprev >> 8) & 0xff);
		audioEncodeBuf[6] = enState->index;
		audioEncodeBuf[7] = 0x00;
        adpcm_coder((short*)pcm, (char*)(audioEncodeBuf+8), shortPcmSize, enState);
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
    
#if DEBUG
    //按字节内容打印调试
    printf("\n%sRTP Header: ", getNowTime().data());
    uint8_t* headerBytes = reinterpret_cast<uint8_t*>(&pkg);
    for (size_t i = 0; i < offsetof(RTP_PKG_HEADER, BCDSIMCardNumber) + audioInfo.BCDSIMLen; ++i) {
        printf("%02X ", headerBytes[i]);
    }
#endif

	if(!(write(audioInfo.socketFd, &pkg, offsetof(RTP_PKG_HEADER, BCDSIMCardNumber)+audioInfo.BCDSIMLen) &&
        write(audioInfo.socketFd, (uint8_t*)&pkg + offsetof(RTP_PKG_HEADER, Bt1LogicChannelNumber), sizeof(RTP_PKG_HEADER) - offsetof(RTP_PKG_HEADER, Bt1LogicChannelNumber))))
    {
		perror("\nerrno:");
		return false;
	}
 
	int count = 0;
	do{
        int size = write(audioInfo.socketFd, audioEncodeBuf+count, BodyLen-count);
		if(size < 0){
			perror("\nerrno:");
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
    }else{
        printf("\n%saudio_type error: %d", getNowTime().data(), audio_type);
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
    //printf("\nsim: %s, avg: %d, count: %d sum:%lld", currentSIM.c_str(), avgAmplitude, sampleCount, sumAbs);
    return avgAmplitude > threshold;
}
