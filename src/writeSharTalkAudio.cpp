#include "writeSharTalkAudio.h"

static std::string mainSIM = std::string();

SharTalkAudio::SharTalkAudio(/* args */)
{
    start_init();
}

SharTalkAudio::~SharTalkAudio()
{
    if (FramHeadPack){
        delete FramHeadPack;
        FramHeadPack = nullptr;
    }
    if(ucOutBuff){
        delete ucOutBuff;
        ucOutBuff = nullptr;
    }
    if (state){
        delete state;
        state = nullptr;
    }
    if(auRtpPtr){
		delete auRtpPtr;
		auRtpPtr = nullptr;
	}
    if(wrOutBuff){
		delete [] wrOutBuff;
		wrOutBuff = nullptr;
	}
    if(state2){
		delete state2;
		state2 = nullptr;
	}
    if(auRtpPtrS){
        delete auRtpPtrS;
        auRtpPtrS = nullptr;
    }
    if(sharHttSer){
        delete sharHttSer;
        sharHttSer = nullptr;
    }
    // if(adpcmStaPtr){
    //     delete adpcmStaPtr;
    //     adpcmStaPtr = nullptr;
    // }
    
    // retur_appcmState_mempoolPtr();
}

void SharTalkAudio::start_init()
{
    FramHeadPack = nullptr;
    FramHeadPack = new FRAM_HEADER();
    timeStampStatus = TIME_STAMP_STATUS_OFF_S;
    timestamp = 0;

    ucOutBuff = new _BYTE[UC_OUT_BUFF_SIZE]();
    state = new adpcm_state();

    auRtpPtr = nullptr;
    auRtpPtr = new AUDIO_HEADER();
    wrOutBuff = new BYTE[WR_OUT_BUFF_SIZE]();
    state2 = new adpcm_state();
    num = 0;
    iRet = 0;
    HeaLeng = 0;
    HeaLeng = sizeof(AUDIO_HEADER);

    auRtpPtrS = nullptr;
    auRtpPtrS = new AUDIO_HEADER_S();
    HeaLengS = 0;
    HeaLengS = sizeof(AUDIO_HEADER_S);
    adpcmStaPtr = new adpcmState(5);  //ADPCM STATE
    sharObjInfoMap.clear();
    gstatus = GET_STATUS_SIRST;
    deStatus = DECOND_STATUS_NO;

    sharHttSer= new sharHttpSer();
    timestatu = TIME_STATUS_START;
    _sim.clear();
}

void SharTalkAudio::reint()
{
    timeStampStatus = TIME_STAMP_STATUS_OFF_S;
    timestamp = 0;

    ucOutbuffSize = 0;
    if(ucOutBuff) memset(ucOutBuff, 0, UC_OUT_BUFF_SIZE);
    if(wrOutBuff) memset(wrOutBuff, 0, WR_OUT_BUFF_SIZE);
    if(auRtpPtr) memset(auRtpPtr, 0, sizeof(AUDIO_HEADER));
    if(state) memset(state, 0, sizeof(adpcm_state));
    if(state2) memset(state2, 0, sizeof(adpcm_state));
    num = 0;
    iRet = 0;
    HeaLeng = 0;
    HeaLeng = sizeof(AUDIO_HEADER);

    if(auRtpPtrS) memset(auRtpPtrS, 0, sizeof(AUDIO_HEADER_S));
    HeaLengS = 0;
    HeaLengS = sizeof(AUDIO_HEADER_S);
    retur_appcmState_mempoolPtr();
    sharObjInfoMap.clear();
    gstatus = GET_STATUS_SIRST;
    deStatus = DECOND_STATUS_NO;
    timestatu = TIME_STATUS_START;
    if(!_sim.empty()){
        delete_deviceID_info(_sim);
        sharHttSer->POST_update(_sim, 4);
        _sim.clear();
    }
}

bool SharTalkAudio::sharInit(std::string sim, SEND_VIDEO_INFO_STRU* infoPtr, _BYTE loadType)
{
    dataInfoPtr = nullptr;
    dataInfoPtr = infoPtr;
    audio_type = (loadType & 0xFF);
    _sim.clear();
    _sim = sim;

    if(mainSIM.empty()){
        mainSIM = _sim;
    }

    strID.clear();
    if(sharHttSer->POST_request(sim, strID)){   //HTTP POST请求
        if(!strID.empty()){
            install_deviceID(sim, strID);
            printf("id: %s\n", strID.data());  
        }
        sharHttSer->POST_update(sim, 1);  //1.链接成功 2.离线  3. 正在进行 4.结束
    }
    
    // exit(0);
    return true;
}


void SharTalkAudio::retur_appcmState_mempoolPtr()
{
    audioType audioInfo2;
    for (const auto& pair : sharObjInfoMap){
        memset(&audioInfo2, 0, sizeof(audioType));
        audioInfo2 = pair.second;
        if(audioInfo2.adpcmState){
            adpcmStaPtr->callback_install(audioInfo2.adpcmState);
        }
    }
    sharObjInfoMap.clear();
}


void SharTalkAudio::add_map()
{
    audioType audioInfo1;
    for (const auto& pair : sharObjInfoMapTmp){
        iter = sharObjInfoMap.find(pair.first);
        if (iter == sharObjInfoMap.end()){
            memset(&audioInfo1, 0, sizeof(audioType));
            audioInfo1 = pair.second;
            audioInfo1.Bt8timeStamp = get_timestamp_S();
            audioInfo1.num = 0;
            if(audioInfo1.Tag_PayloadType == 0x9A) {
                audioInfo1.adpcmState = adpcmStaPtr->get_memblock();
                // printf("-------------get adpcmStaPtr----------------\n");
            }
            audioInfo1.index = 0;
            sharObjInfoMap[pair.first] = audioInfo1;
        }
    } 
}

void SharTalkAudio::alter_map(std::string sim)
{
    audioType audioInfo;
    memset(&audioInfo, 0, sizeof(audioType));
    _iter = sharObjInfoMap.find(sim);
    if(_iter != sharObjInfoMap.end()){
        audioInfo = _iter->second;
        audioInfo.Bt8timeStamp += 3;
        audioInfo.num += 1;
        audioInfo.index = state2->index;  
        _iter->second  = audioInfo;

        //printf("----- audioInfo1.Bt8timeStamp = %ld, sim = %s\n", audioInfo.Bt8timeStamp, sim.c_str());
    }
}

unsigned long SharTalkAudio::get_timestamp_S()
{
    struct timeval tvS;
	unsigned long int timestampS;
    gettimeofday(&tvS, NULL);
	timestampS = (tvS.tv_sec * 1000 + tvS.tv_usec / 1000);
    return timestampS;
}

void SharTalkAudio::add_workmap()
{
    audioType audioInfo2;
    for (const auto& Tpair : sharObjInfoMapTmp){
        auto it = std::find(ulist.begin(), ulist.end(), Tpair.first);
        if(it != ulist.end()){
            iter = sharObjInfoMap.find(Tpair.first);
            if (iter == sharObjInfoMap.end()){
                memset(&audioInfo2, 0, sizeof(audioType));
                audioInfo2 = Tpair.second;
                audioInfo2.Bt8timeStamp = get_timestamp_S();
                audioInfo2.num = 0;
                if(audioInfo2.Tag_PayloadType == 0x9A) {
                    audioInfo2.adpcmState = adpcmStaPtr->get_memblock();
                    // printf("-------------get adpcmStaPtr----------------\n");
                }
                audioInfo2.index = 0;
                sharObjInfoMap[Tpair.first] = audioInfo2;
            }
        }
    }
}

bool SharTalkAudio::Get_deviceIdMAP()
{
    ulist.clear();
    udeviInfomap.clear();
    sharObjInfoMapTmp.clear();
    get_allDeviceID(udeviInfomap);
    if(udeviInfomap.size() > 1){
        for (const auto& upair : udeviInfomap){
            if(upair.second == strID){     //获取ID相同的设备信息
                ulist.push_back(upair.first);  //添加到链表
            }
        }
    }else{
        return false;
    }

    get_audio_type_info2(sharObjInfoMapTmp);
    if(sharObjInfoMapTmp.size() > 1){
        add_workmap();
    }else {
        return false;
    }
    return true;
}

bool SharTalkAudio::first_getDeviceInfo()
{
    time_t start_u, end_u;
    time(&start_u);
    for(;;){
        if(Get_deviceIdMAP()){
            return true;
        }
        usleep(20000);
        time(&end_u);
        double diff_t = difftime(end_u, start_u);
        if(diff_t > 5) {     //5秒未获取到设备信息退出
            return false;
        }
    }
}

bool SharTalkAudio::device_status_info()
{
    if(TIME_STATUS_START == timestatu){
        if(!(first_getDeviceInfo())){
            printf("============超时=============\n");
            return false;
        }
        time(&start_t); 
        timestatu = TIME_STATUS_END;
        printf("=================sleep(5)==============\n");
    }
    time(&end_t);
    double diff_t = difftime(end_t, start_t);
    
    if(diff_t > 8){
        Get_deviceIdMAP();
        time(&start_t);
    }

    return true;
}

bool SharTalkAudio::write_shar_device()
{
    if (!audio_decoder()) printf("audio_type fail!\n");
    // if(!(device_status_info());  //未获取到设备信息退出

    if(TIME_STATUS_START == timestatu){
        time(&start_t); 
        timestatu = TIME_STATUS_END;
        printf("=================sleep(5)==============\n");
    }
    time(&end_t);
    double diff_t = difftime(end_t, start_t);

    if(_sim != mainSIM){
        return false;
    }
    //printf("mainSIM:%s, size: %d\n", mainSIM.c_str(), sharObjInfoMap.size());

    // if(diff_t < 8){
        //printf("=================diff_t < 8==============\n");
        sharObjInfoMapTmp.clear();
        get_audio_type_info2(sharObjInfoMapTmp);
        add_map();
    // }
    

    for ( iter = sharObjInfoMap.begin(); iter != sharObjInfoMap.end(); ++iter){
        if(iter->first == _sim) continue;
   
        sims.clear();
        sims = iter->first;
        memset(&audioTypeInfo, 0, sizeof(audioType));
        audioTypeInfo = iter->second;
        if(10 == audioTypeInfo.BCDSIMLen){
            push_to_device_SIM10();
            alter_map(sims);
            //printf("ucOutbuffSize = %d, sim = %s\n", ucOutbuffSize, _sim.c_str());
        }
        else if (6 == audioTypeInfo.BCDSIMLen){
            push_to_device_SIM6();
            alter_map(sims);
            //printf("audioTypeInfo.BCDSIMLen == 6\n");
        }
        // audio_DeEncoder_judge();
    }

    // deStatus = DECOND_STATUS_NO;
}

bool SharTalkAudio::audio_DeEncoder_judge()
{
    Load_Type = (audioTypeInfo.Tag_PayloadType & 0x7F);
    if(Load_Type == audio_type){  //负载类型相同
        audio_Load_equal();
        printf("-----------audio_Load_equal--------\n");
    }else{
        audio_Load_inequality();    //负载类型不同
         printf("-----------audio_Load_inequality--------\n");
    }
    return true;
}

bool SharTalkAudio::audio_Load_equal()
{
    if(!dataInfoPtr) return false;

    BodyLen = 0;
    wrLoadBuff = nullptr;
    BodyLen = dataInfoPtr->WdBodyLen;
    wrLoadBuff = dataInfoPtr->VidePacData;
    if(10 == audioTypeInfo.BCDSIMLen){
        audio_NoDePack_SIM10();
        alter_map(sims);
    }else if (6 == audioTypeInfo.BCDSIMLen){
        audio_NoDePack_SIM6();
        alter_map(sims);
    }
    return true;
}

bool SharTalkAudio::audio_Load_inequality()
{
    if(DECOND_STATUS_NO == deStatus){
        if (!audio_decoder()) printf("audio_type fail!\n");   //解码音频
        deStatus = DECOND_STATUS_YES;
    }
    
    if(10 == audioTypeInfo.BCDSIMLen){
        push_to_device_SIM10();
        alter_map(sims);
        // printf("ucOutbuffSize = %d, sim = %s\n", ucOutbuffSize, sims.c_str());
    }
    else if (6 == audioTypeInfo.BCDSIMLen){
        push_to_device_SIM6();
        alter_map(sims);
        // printf("audioTypeInfo.BCDSIMLen == 6\n");
    }

    return true;
}


bool SharTalkAudio::audio_NoDePack_SIM10()
{
    if(!auRtpPtr) return false;
	 
	// memset(auRtpPtr, 0, sizeof(AUDIO_HEADER));
	// auRtpPtr->DWFramHeadMark = 0x64633130;
	auRtpPtr->info1          = 0x81;
	auRtpPtr->info2          = audioTypeInfo.Tag_PayloadType;              
	memcpy(auRtpPtr->BCDSIMCardNumber, audioTypeInfo.BCDSIMCardNumber, 10);
	auRtpPtr->Bt1LogicChannelNumber = audioTypeInfo.ChannelNumber;
	auRtpPtr->info3          = 0x30;
    auRtpPtr->WdBodyLen = htons(BodyLen);
    auRtpPtr->WdPackageSequence = htons(audioTypeInfo.num); 
	auRtpPtr->Bt8timeStamp = htonl(audioTypeInfo.Bt8timeStamp); 

	write_data_SIM10();

    return true;
}

bool SharTalkAudio::audio_NoDePack_SIM6()
{
    if(!auRtpPtrS){
        printf("===========auRtpPtrS = nullptr===============\n");
        return false;
    }

	// memset(auRtpPtrS, 0, sizeof(AUDIO_HEADER_S));
	// auRtpPtrS->DWFramHeadMark = 0x64633130;
	auRtpPtrS->info1          = 0x81;
	auRtpPtrS->info2          = audioTypeInfo.Tag_PayloadType;              
	memcpy(auRtpPtrS->BCDSIMCardNumber, audioTypeInfo.BCDSIMCardNumber, 6);
	auRtpPtrS->Bt1LogicChannelNumber = audioTypeInfo.ChannelNumber;
	auRtpPtrS->info3          = 0x30;

    auRtpPtrS->WdBodyLen = htons(BodyLen);
    auRtpPtrS->WdPackageSequence = htons(audioTypeInfo.num); 
    auRtpPtrS->Bt8timeStamp = htonl(audioTypeInfo.Bt8timeStamp); 

    write_data_SIM6();
    return true;
}


bool SharTalkAudio::push_to_device_SIM6()
{
	if(!auRtpPtrS){
        printf("======436=====auRtpPtrS = nullptr===============\n");
        return false;
    }
	 
    // printf("======440=====auRtpPtrS != nullptr===============\n");
	// memset(auRtpPtrS, 0, sizeof(AUDIO_HEADER_S));
	// auRtpPtrS->DWFramHeadMark = 0x64633130;
	auRtpPtrS->info1          = 0x81;
	auRtpPtrS->info2          = audioTypeInfo.Tag_PayloadType;              
	memcpy(auRtpPtrS->BCDSIMCardNumber, audioTypeInfo.BCDSIMCardNumber, 6);
	auRtpPtrS->Bt1LogicChannelNumber = audioTypeInfo.ChannelNumber;
	auRtpPtrS->info3          = 0x30;

    BodyLen = 0;
	memset(wrOutBuff, 0, WR_OUT_BUFF_SIZE);
	if(audioTypeInfo.Tag_PayloadType == 0x86){
		iRet = g711a_encode(wrOutBuff, (short*)ucOutBuff, ucOutbuffSize);
		BodyLen = (unsigned short)(ucOutbuffSize / 2);
	}else if(audioTypeInfo.Tag_PayloadType == 0x9A){
		memcpy(wrOutBuff,audioTypeInfo.ADPCM_8, 4);
		wrOutBuff[4] = (state2->valprev & 0xff);
		wrOutBuff[5] = ((state2->valprev >> 8) & 0xff);
		wrOutBuff[6] = state2->index;
		wrOutBuff[7] = 0x00;
		adpcm_coder((short*)ucOutBuff, (char*)(wrOutBuff+8), ucOutbuffSize / 2, state2);
		BodyLen = (ucOutbuffSize / 4) + 8;
        // printf("======461====BodyLen = %d\n" , BodyLen);

	}else{
        return false;
    }
    wrLoadBuff = nullptr;
    wrLoadBuff = wrOutBuff;

	auRtpPtrS->WdBodyLen = htons(BodyLen);

	// num++;
	// auRtpPtrS->WdPackageSequence = htons(num++);
    auRtpPtrS->WdPackageSequence = htons(audioTypeInfo.num); //num++
	get_timestamp();

	// auRtpPtrS->Bt8timeStamp = htonl(timestamp);
    auRtpPtrS->Bt8timeStamp = htonl(audioTypeInfo.Bt8timeStamp); //timestamp

	return write_data_SIM6();
}

bool SharTalkAudio::push_to_device_SIM10()
{
	if(!auRtpPtr) return false;
	 
	// memset(auRtpPtr, 0, sizeof(AUDIO_HEADER));
	// auRtpPtr->DWFramHeadMark = 0x64633130;
	auRtpPtr->info1          = 0x81;
	auRtpPtr->info2          = audioTypeInfo.Tag_PayloadType;              
	memcpy(auRtpPtr->BCDSIMCardNumber, audioTypeInfo.BCDSIMCardNumber, 10);
	auRtpPtr->Bt1LogicChannelNumber = audioTypeInfo.ChannelNumber;
	auRtpPtr->info3          = 0x30;

    BodyLen = 0;
	memset(wrOutBuff, 0, WR_OUT_BUFF_SIZE);
	if(audioTypeInfo.Tag_PayloadType == 0x86){
		iRet = g711a_encode(wrOutBuff, (short*)ucOutBuff, ucOutbuffSize);
		BodyLen = (unsigned short)(ucOutbuffSize / 2);
	}else if(audioTypeInfo.Tag_PayloadType == 0x9A){
		memcpy(wrOutBuff,audioTypeInfo.ADPCM_8, 4);
		wrOutBuff[4] = (state2->valprev & 0xff);
		wrOutBuff[5] = ((state2->valprev >> 8) & 0xff);
		wrOutBuff[6] = state2->index;
		wrOutBuff[7] = 0x00;
		adpcm_coder((short*)ucOutBuff, (char*)(wrOutBuff+8), ucOutbuffSize / 2, state2);
		BodyLen = (ucOutbuffSize / 4) + 8;

	}else{
        return false;
    }
    wrLoadBuff = nullptr;
    wrLoadBuff = wrOutBuff;

	auRtpPtr->WdBodyLen = htons(BodyLen);

	// num++;
	// auRtpPtr->WdPackageSequence = htons(num++); //num++
    auRtpPtr->WdPackageSequence = htons(audioTypeInfo.num); //num++
	get_timestamp();

    // auRtpPtr->Bt8timeStamp = htonl(timestamp); 
	auRtpPtr->Bt8timeStamp = htonl(audioTypeInfo.Bt8timeStamp); //timestamp

	return write_data_SIM10();
}

bool SharTalkAudio::write_data_SIM10()
{
	wriRet = 0;
    printf("============== auRtpPtr->DWFramHeadMark  = %X\n", auRtpPtr->DWFramHeadMark);

	wriRet = write(audioTypeInfo.socketFd, auRtpPtr, HeaLeng);
	if(wriRet < 0){
        close_wrirteFd();
		perror("errno:");
		return false;
	}

	count = 0;
	wriRet = 0;

	do{
        wriRet = write(audioTypeInfo.socketFd, wrLoadBuff+count, BodyLen-count);
		if(wriRet < 0){
            close_wrirteFd();
			perror("errno:");
			return false;
		}
		count += wriRet;
	}while ((0 < count) && (count<BodyLen));

	return true;
}

bool SharTalkAudio::write_data_SIM6()
{
    wriRet = 0;
    //printf("============== HeaLengSSIM6 = %d\n", HeaLengS);

	wriRet = write(audioTypeInfo.socketFd, auRtpPtrS, HeaLengS);
	if(wriRet < 0){
        close_wrirteFd();
		perror("errno:");
		return false;
	}

	count = 0;
	wriRet = 0;

	do{
        wriRet = write(audioTypeInfo.socketFd, wrLoadBuff+count, BodyLen-count);
		if(wriRet < 0){
            close_wrirteFd();
			perror("errno:");
			return false;
		}
		count += wriRet;
	}while ((0 < count) && (count<BodyLen));

	return true;
}


void SharTalkAudio::close_wrirteFd()
{
    for (const auto& pair : sharObjInfoMap){
        close(pair.second.socketFd);
    }
}

bool SharTalkAudio::get_timestamp()
{
	if(timeStampStatus == TIME_STAMP_STATUS_OFF_S){
		timeStampStatus = TIME_STAMP_STATUS_ON_S;
		gettimeofday(&tv, NULL);
		timestamp = (tv.tv_sec * 1000 + tv.tv_usec / 1000);
		return true;
	}

	timestamp += 3;
	return true;
}

bool SharTalkAudio::audio_decoder()
{
    if(!dataInfoPtr) return false;
    if(!ucOutBuff) return false;

    ucOutbuffSize = 0;
    memset(ucOutBuff, 0, UC_OUT_BUFF_SIZE);
    if(audio_type == 0x06){
        if(!G711A_decode()) return false;
    }else if(audio_type == 0x1A){ 
        if(!ADPCM_decode()) return false;
    }else{
        return false;
    }

    return true;
}


bool SharTalkAudio::G711A_decode()
{
    if(!dataInfoPtr) return false;

    if(dataInfoPtr->VidePacData){
        ucOutbuffSize = g711a_decode((short*)ucOutBuff, dataInfoPtr->VidePacData, dataInfoPtr->WdBodyLen);
    }

    return true;
}

bool SharTalkAudio::ADPCM_decode()
{

    if(!state || !dataInfoPtr) return false;

    len = 0;
    coun = 0;
    dataPtr = nullptr;
    // memset(state, 0, sizeof(adpcm_state));
    dataPtr = dataInfoPtr->VidePacData;
    len = dataInfoPtr->WdBodyLen;

    if (dataPtr[0] == 0x00 && dataPtr[1] == 0x01 && (dataPtr[2] & 0xff) == (len - 4) / 2 && dataPtr[3] == 0x00){
        state->valprev = (short)(((dataPtr[5] << 8) & 0xff) | (dataPtr[4] & 0xff));
        state->index = dataPtr[6];
        adpcm_decoder((char*)(dataPtr +8), (short*)ucOutBuff, (len - 8) * 2, state);
        ucOutbuffSize = (len - 8) * 4;

       if(isSpeechPresent((short*)ucOutBuff, ucOutbuffSize/sizeof(short), 400)) {
            printf("isSpeechPresent: sim: %s, mainSIM: %s, size:%d\n", _sim.c_str(), mainSIM.c_str(), sharObjInfoMap.size());
            if(_sim != mainSIM) {
                mainSIM = _sim;
                memset(state, 0, sizeof(adpcm_state));
            }
        }

        // printf("------------ucOutbuffSize-= %d ----- ucOutbuffSize----= %d --- len = %d, state->index = %d\n", dataPtr[7], ucOutbuffSize, len, state->index);
        return true;
    }

    state->valprev = (short)(((dataPtr[1] << 8) & 0xff) | (dataPtr[0] & 0xff));
    state->index = dataPtr[2];
    adpcm_decoder((char*)(dataPtr +4), (short*)ucOutBuff, (len - 4) * 2, state);
    ucOutbuffSize = (len - 4) * 4;

    if(isSpeechPresent((short*)ucOutBuff, ucOutbuffSize/sizeof(short), 400)) {
         printf("isSpeechPresent: sim: %s, mainSIM: %s, size:%d\n", _sim.c_str(), mainSIM.c_str(), sharObjInfoMap.size());
         if(_sim != mainSIM) {
            mainSIM = _sim; 
            memset(state, 0, sizeof(adpcm_state));
        }
    }
   
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
    //printf("\nsim: %s, avg: %d, count: %d sum:%lld\n", _sim.c_str(), avgAmplitude, sampleCount, sumAbs);
    return avgAmplitude > threshold;
}