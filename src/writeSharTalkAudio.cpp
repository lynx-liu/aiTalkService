#include <unistd.h>
#include <cstddef>
#include <cstring>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <mutex>
#include <thread>
#include "debug.h"
#include "tiny_ws.h"
#include "writeSharTalkAudio.h"

// declare libfvad C API we'll use (libfvad lives in ../libfvad)
extern "C" {
    struct Fvad;
    Fvad *fvad_new(void);
    void fvad_free(Fvad *inst);
    int fvad_set_mode(Fvad* inst, int mode);
    int fvad_set_sample_rate(Fvad* inst, int sample_rate);
    int fvad_process(Fvad* inst, const int16_t* frame, size_t length);
}

#define WAIT_MAIN_SIM_TIME  2000 //ms
#define PCM_FRAME_SIZE      640
#define SPEECH_ON_PACKET_COUNT  15 // isSpeechPresent 连续为真，判定开始讲话
#define SPEECH_OFF_PACKET_COUNT 50 // isSpeechPresent 连续为假，判定停止讲话

#define TYPE_WS_WEB_TALK    0x00 //通过websocket平台对讲
#define TYPE_GROUP_TALK     0x01 //群组对讲
#define TYPE_AI_TALK        0x02 //AI对讲
#define TYPE_WS_VAR_TALK    0x04 //多变量通知触发的对讲

struct GroupAudioInfo {
    std::mutex mtx;
    std::string mainSIM;
    uint64_t timestamp;
    std::map<std::string, audioType> simMap;
};

static std::map<std::string, std::shared_ptr<GroupAudioInfo>> sharObjInfoMap;
static std::mutex g_group_map_mutex;

SharTalkAudio::SharTalkAudio(const CONFIG ServerConfig)
{
    pAudioDenoiser = new AudioDenoiser(8000);

    ucOutBuff = new uint8_t[BUFF_SIZE]();
    deState = new adpcm_state();

    audioEncodeBuf = new uint8_t[BUFF_SIZE]();
    enState = new adpcm_state();

    sharHttSer= new sharHttpSer(ServerConfig);
    currentSIM.clear();
    speakingStartTime_ = 0;
    speechPktCounter_ = 0;
    silencePktCounter_ = 0;

    // create and configure libfvad instance (aggressive mode to avoid false positives)
    vad = fvad_new();
    if (vad) {
        fvad_set_sample_rate(vad, 8000);
        fvad_set_mode(vad, 3);
    }
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
    if (vad) {
        fvad_free(vad);
        vad = nullptr;
    }
}

void SharTalkAudio::reint()
{
    isSpeaking = false;
    speakingStartTime_ = 0;
    speechPktCounter_ = 0;
    silencePktCounter_ = 0;
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

        std::lock_guard<std::mutex> lock(pcm_mutex);
        {
            std::lock_guard<std::mutex> mapLock(g_group_map_mutex);
            auto groupIt = sharObjInfoMap.find(groupID);
            if (groupIt != sharObjInfoMap.end() && groupIt->second) {
                auto groupInfo = groupIt->second;
                std::lock_guard<std::mutex> groupLock(groupInfo->mtx);
                auto& simMap = groupInfo->simMap;

                simMap.erase(currentSIM);
                printf("\n%sremove SIM: %s from groupID: %s (reint), currentGroup size: %zu", getNowTime().data(), currentSIM.data(), groupID.data(), simMap.size());

                if (groupInfo->mainSIM == currentSIM) {
                    if (!simMap.empty()) {
                        groupInfo->mainSIM = simMap.begin()->first;
                        groupInfo->timestamp = get_timestamp();
                        sharHttSer->updateTalkingState(groupInfo->mainSIM, 5);  //1.链接成功 2.离线  3. 正在进行 4.结束, 5. 成为主讲人
                    } else {
                        groupInfo->mainSIM.clear();  // 当前组为空了
                    }
                }

                if (simMap.empty()) {
                    sharObjInfoMap.erase(groupIt);
                    printf("\n%sremove groupID: %s (reint), group size: %zu", getNowTime().data(), groupID.data(), sharObjInfoMap.size());
                }
            }

            if(type&TYPE_GROUP_TALK) {
                sharHttSer->updateTalkingState(currentSIM, 4);
            }
            currentSIM.clear();
        }
    }
}

bool SharTalkAudio::sharInit(std::string sim, uint8_t loadType)
{
    currentSIM = sim;
    audio_type = loadType&0x7F;

    isSpeaking = false;
    speakingStartTime_ = 0;
    speechPktCounter_ = 0;
    silencePktCounter_ = 0;
    offset = 0;
    wsRecvPcm.clear();
    httpRecvPcm.clear();
    pcmBuf.clear();

    groupID.clear();
    type = 0;
    pkgCnt = 0;
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

                    memset(self->deState, 0, sizeof(adpcm_state));
                    memset(self->enState, 0, sizeof(adpcm_state));

                    {
                        std::lock_guard<std::mutex> mapLock(g_group_map_mutex);
                        auto groupIt = sharObjInfoMap.find(self->groupID);
                        if (groupIt != sharObjInfoMap.end() && groupIt->second) {
                            auto groupInfo = groupIt->second;
                            std::lock_guard<std::mutex> groupLock(groupInfo->mtx);
                            auto& simMap = groupInfo->simMap;

                            simMap.erase(self->currentSIM);
                            printf("\n%sremove SIM: %s from groupID: %s (ws disconnected), currentGroup size: %zu", getNowTime().data(), self->currentSIM.data(), self->groupID.data(), simMap.size());

                            if (simMap.empty()) {
                                groupInfo->mainSIM.clear();  // 当前组为空了
                                sharObjInfoMap.erase(groupIt);
                                printf("\n%sremove groupID: %s (ws disconnected), group size: %zu", getNowTime().data(), self->groupID.data(), sharObjInfoMap.size());
                            }
                        }
                    }

                    if(!self->currentSIM.empty()) {
                        int newType = 0;
                        if(self->sharHttSer->getTalkingInfo(self->currentSIM, self->groupID, newType)) { //重新获取对讲信息
                            self->type |= newType;
                            printf("\n%sgroupID: %s, type: %d --> %d", getNowTime().data(), self->groupID.data(), newType, self->type);

                            if(!self->groupID.empty()){
                                self->add_map(self->currentSIM, self->groupID);
                            }
                        }
                    }
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
    std::shared_ptr<GroupAudioInfo> groupInfo;
    size_t groupCount = 0;
    {
        // add_map 低频：允许持有全局锁时间稍长，保证不会与删除竞争导致“加到已被erase的组对象”
        std::lock_guard<std::mutex> mapLock(g_group_map_mutex);
        auto& groupPtr = sharObjInfoMap[groupId];
        if (!groupPtr) {
            groupPtr = std::make_shared<GroupAudioInfo>();
            groupPtr->timestamp = 0;
        }
        groupInfo = groupPtr;
        groupCount = sharObjInfoMap.size();
    }

    std::lock_guard<std::mutex> groupLock(groupInfo->mtx);
    auto& simMap = groupInfo->simMap;

    if (simMap.find(sim) != simMap.end())
        return;

    audioType audioInfo;
    memset(&audioInfo, 0, sizeof(audioType));
    get_audio_type_info(sim, audioInfo);
    audioInfo.Bt8timeStamp = get_timestamp();
    simMap[sim] = audioInfo;

    if (groupInfo->mainSIM.empty()) {
        groupInfo->mainSIM = sim;
        groupInfo->timestamp = get_timestamp();
        if(type&TYPE_GROUP_TALK) {
            sharHttSer->updateTalkingState(groupInfo->mainSIM, 5);  //1.链接成功 2.离线  3. 正在进行 4.结束, 5. 成为主讲人
        }
    }

    printf("\n%sgroupID: %s add SIM: %s | group size: %zu, currentGroup size: %zu", getNowTime().data(), groupId.c_str(), sim.c_str(), groupCount, simMap.size());
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

    // 只在查找 group 时持有全局锁：不同 group 可以并发执行
    std::shared_ptr<GroupAudioInfo> groupInfo;
    {
        std::lock_guard<std::mutex> mapLock(g_group_map_mutex);
        auto groupIt = sharObjInfoMap.find(groupID);
        if (groupIt == sharObjInfoMap.end() || !groupIt->second) {
            return false;
        }
        groupInfo = groupIt->second;
    }

    pAudioDenoiser->denoiseBuffer((short*)ucOutBuff, shortPcmSize);

    // 注意锁顺序：先 pcm_mutex 再 groupInfo->mtx，避免与 reint/on_disconnect 形成死锁环
    std::lock_guard<std::mutex> lock(pcm_mutex);//确保webSocketFd和type线程同步更新，保护simMap遍历时不被修改，以及wsRecvPcm和httpRecvPcm数据一致性

    // 组内锁：保护 mainSIM/timestamp/simMap 以及 simMap 遍历/修改
    std::lock_guard<std::mutex> groupLock(groupInfo->mtx);
    auto& simMap = groupInfo->simMap;

    int64_t currentTime = get_timestamp();
    SpeechTransition st = updateSpeakingState((short*)ucOutBuff, shortPcmSize);

    if (st.started) {
        printf("\n%s%s isSpeaking = true", getNowTime().data(), currentSIM.c_str());
        speakingStartTime_ = currentTime;

        if(type&TYPE_AI_TALK || type&TYPE_WS_VAR_TALK) {
            pcmBuf.clear();
        }
        refreshMainSIM(groupInfo, currentTime);
    } else if (st.stopped) {
        printf("\n%s%s isSpeaking = false", getNowTime().data(), currentSIM.c_str());

        if(type&TYPE_AI_TALK || type&TYPE_WS_VAR_TALK) {
            int64_t duration = currentTime - speakingStartTime_;
            if(duration >= WAIT_MAIN_SIM_TIME) {
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
            } else {
                printf("\n%s%s speech too short (%ldms), skip POST_pcm", getNowTime().data(), currentSIM.c_str(), (long)duration);
            }
        }
    } else if (st.speaking) {
        refreshMainSIM(groupInfo, currentTime);
    }

    for (auto& simPair : simMap) {
        const std::string& sim = simPair.first;
        audioType& audioInfo = simPair.second;

        if (sim == currentSIM) {
            if(pkgCnt++==0) printf(" %d", type);
            if(webSocketFd>0) {//ws连上了，可能是多变量对讲，也可能是平台对讲
                wsplayback(audioInfo, ucOutBuff, shortPcmSize);
            }
            
            if(type&TYPE_WS_VAR_TALK || type&TYPE_AI_TALK) {//多变量对讲 或 AI对讲
                httpplayback(audioInfo, ucOutBuff, shortPcmSize);
            }
            continue;
        }

        if(currentSIM != groupInfo->mainSIM){
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

bool SharTalkAudio::isSpeechPresent(const short* pcm, int sampleCount)
{
    int idx = 0;
    int total_frames = 0;
    int voiced_frames = 0;
    int FRAME_SIZE = 160; // 20ms at 8kHz

    while (sampleCount - idx >= FRAME_SIZE) {
        if (fvad_process(vad, pcm + idx, FRAME_SIZE) > 0)
            voiced_frames++;
        total_frames++;
        idx += FRAME_SIZE;
    }
    return total_frames>1 && voiced_frames==total_frames;
}

SpeechTransition SharTalkAudio::updateSpeakingState(const short* pcm, int sampleCount)
{
    SpeechTransition st = {false, false, isSpeaking};

    if (isSpeechPresent(pcm, sampleCount)) {
        speechPktCounter_++;
        silencePktCounter_ = 0;
    } else {
        silencePktCounter_++;
        speechPktCounter_ = 0;
    }

    if (!isSpeaking && speechPktCounter_ >= SPEECH_ON_PACKET_COUNT) {
        isSpeaking = true;
        st.started = true;
        st.speaking = true;
    } else if (isSpeaking && silencePktCounter_ >= SPEECH_OFF_PACKET_COUNT) {
        isSpeaking = false;
        speechPktCounter_ = 0;
        silencePktCounter_ = 0;
        st.stopped = true;
        st.speaking = false;
    } else {
        st.speaking = isSpeaking;
    }

    return st;
}

void SharTalkAudio::refreshMainSIM(const std::shared_ptr<GroupAudioInfo>& groupInfo, int64_t currentTime)
{
    if(currentSIM != groupInfo->mainSIM) {
        if(currentTime-groupInfo->timestamp>WAIT_MAIN_SIM_TIME) {
            groupInfo->mainSIM = currentSIM;
            groupInfo->timestamp = currentTime;
            std::string sim = groupInfo->mainSIM;
            std::thread([this, sim](){
                sharHttSer->updateTalkingState(sim, 5);  //1.链接成功 2.离线  3. 正在进行 4.结束, 5. 成为主讲人
            }).detach();

            memset(deState, 0, sizeof(adpcm_state));
            memset(enState, 0, sizeof(adpcm_state));
        }
    } else {
        groupInfo->timestamp = currentTime;
    }
}
