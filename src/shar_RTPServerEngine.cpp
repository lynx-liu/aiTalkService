#include "shar_RTPServerEngine.h"
using namespace Cnvt;

EngineServerJTT::EngineServerJTT(const CONFIG ServerConfig, int fd)
{
	UrlKey = ServerConfig.UrlKey;
	urlDNS.clear();
	urlDNS = ServerConfig.urlDNS;
	                       
	start_init();
}

EngineServerJTT::~EngineServerJTT()
{

}

void EngineServerJTT::start_init()
{
	RecvRtpPackStr = new SAVER_RECV_DATA_SIM6();
	RecvRtpPackStr->PackStatus = _PKG_HD_INIT;

	RreadReturnLen   = 0;

	SIMStatus = CREATE_SIM_STATUS_OFF;
	status = CREATE_AUDIO_STATUS_OFF;

	_memptr = nullptr;
	memsize = 0;
	_size = 0;
	capacity = 0;
	_getStatus = _NEW_HEAP_STATUS_OFF;
	mempoolStatus = MEMPOOL_INIT_STATUS_OFF;
	_memptr = (_BYTE*)malloc(MEMPTR_SIZE);
	memset(_memptr, 0, MEMPTR_SIZE);
	memsize = MEMPTR_SIZE;
	urlInitStatus = RTMP_URL_INIT_STATUS_OFF;

	audioPtr = nullptr;
	audioPtr = (_BYTE*)malloc(AUDIO_BUFF_SIZE);
	memset(audioPtr,0, AUDIO_BUFF_SIZE);

	dataStructPtr = nullptr;
	dataStructPtr = new SEND_VIDEO_INFO_STRU();

	rtmpSendObj = new RtmpSender();

	CsockFd = 0;

	talkStatus = TALK_STATUS_FIST;
	// sharTalkdataPtr = nullptr;
	// sharTalkdataPtr = new SHAR_TALK_DATA_TYPE();

	_sharTalkstrue = new SharTalkAudio();
}

void EngineServerJTT::close_and_free()
{
	close(CsockFd);
	if(rtmpSendObj){
		rtmpSendObj->close_free();
		delete rtmpSendObj;
		rtmpSendObj = nullptr;
	}

	if(nullptr != RecvRtpPackStr){
		delete RecvRtpPackStr;
		RecvRtpPackStr = nullptr;
	}

	if(nullptr != _memptr){
		free(_memptr);
		_memptr = nullptr;
	}
	if(nullptr != audioPtr){
		free(audioPtr);
		audioPtr = nullptr;
	}

	if(dataStructPtr){
		delete dataStructPtr;
		dataStructPtr = nullptr;
	}
	// if(sharTalkdataPtr){
	// 	delete sharTalkdataPtr;
	// 	sharTalkdataPtr = nullptr;
	// }
	if(_sharTalkstrue){
		delete _sharTalkstrue;
		_sharTalkstrue = nullptr;
	}
}

void EngineServerJTT::init(int fd)
{
	// if(fd<0) LOG(ERROR)<< "fd < 0 ERROR: fd= "<< fd;
	CsockFd = fd;
	PackStatus_ = _PKG_HD_INIT;
	PackHeadLen_ = 0;
}

void EngineServerJTT::reInit()
{
	rtmpSendObj->reInit();
	urlInitStatus = RTMP_URL_INIT_STATUS_OFF;

	if(nullptr != RecvRtpPackStr){
		memset(RecvRtpPackStr, 0, sizeof(SAVER_RECV_DATA_SIM6));
		RecvRtpPackStr->PackStatus = _PKG_HD_INIT;
	}else {
		// LOG(ERROR) << "reInit: nullptr == RecvRtpPackStr";
	}
	
	PackStatus_ = _PKG_HD_INIT;
	SIMStatus = CREATE_SIM_STATUS_OFF;
	if(nullptr != _memptr){
		memset(_memptr, 0, memsize);
		_size = 0;
	}

	close(CsockFd);
	// CsockFd = 0;

	talkStatus = TALK_STATUS_FIST;
	memset(audioPtr,0, AUDIO_BUFF_SIZE);
	pTemp = nullptr;
	pTemp2 = nullptr;
	pTemp3 = nullptr;

	_sharTalkstrue->reint();
	
}

bool EngineServerJTT::create_rtmpSendObj()
{
	char RTMPAddr[256] = {'\0'};
	CreateExeclFfmpegCourse(/*BCDSIMStr_t, BCDChanStr_t*/);
	// sprintf(RTMPAddr, "rtmp://%s/live/%s?txSecret=%s&txTime=%s", (char*)urlDNS.c_str(), \
	// (char*)StreamName.c_str(), (char*)EncryStr.c_str(), (char*)RetTime_t.c_str());
	
	sprintf(RTMPAddr, "rtmp://127.0.0.1:3935/live/%s", (char*)StreamName.c_str());

	int  count = 0;
	for(;;){
		count++;

		if(rtmpSendObj->init(RTMPAddr, dataStructPtr)) return true;
		if(count >=20) return false;

		// LOG(ERROR)<< "RTMP URL INIT FAIL! "<< StreamName;
	}
	return true;
}

bool EngineServerJTT::sendDataV3()
{

	if(urlInitStatus == RTMP_URL_INIT_STATUS_OFF){
		if(!create_rtmpSendObj()) return false;
		urlInitStatus = RTMP_URL_INIT_STATUS_ON;
	}
	rtmpSendObj->executeProcess();

	return true;
}

 // 函数说明:防盗链KEYmd5加密
string EngineServerJTT::MD5Encryption(const string& WaitEncryStr)
{
	std::string md5_string;
	unsigned char md[16] = { 0 };
	char tmp[33] = { 0 };
	MD5_CTX ctx;
	MD5_Init( &ctx );
	MD5_Update( &ctx, (unsigned char*)WaitEncryStr.c_str(), WaitEncryStr.size() );
	MD5_Final( md, &ctx );

	for( int i = 0; i < 16; ++i ){   
		memset( tmp, 0x00, sizeof( tmp ) );
		sprintf( tmp, "%02x", md[i] );
		md5_string += tmp;
	}

	return md5_string;
}

//获取十六进制推流有效时间 (如:5DD435ED) 默认当前时间*12小时
string EngineServerJTT::GetHushStreamAddEffectiveTime()
{
	char time_16[sizeof(time_t)+1];
	time_t timep;
	time(&timep);
	timep += (60*60*12);
	memset(time_16, 0, sizeof(time_16));
	sprintf(time_16, "%X", timep);

	std::string Retime = time_16;

	return Retime;
}

bool EngineServerJTT::CreateExeclFfmpegCourse()
{
	RetTime_t.clear();
	txSecret.clear();
	EncryStr.clear();

	/*获取有效时间戳*/
	RetTime_t = GetHushStreamAddEffectiveTime();
	txSecret += UrlKey;
	txSecret += StreamName;
	txSecret += RetTime_t;
	//	txSecret = UrlKey + StreamName + RetTime_t;

	/*MD5url防盗链*/
	EncryStr = MD5Encryption(txSecret);

	return true;
}

bool EngineServerJTT::ReadAndAnalyzeRTPPack()
{
	// char buff[1024];
	for(;;){
		gRecvLen = 0;

		gRecvLen = ReadPackLen_g();
		if(-1 == gRecvLen || 0 == gRecvLen){
			// LOG(ERROR)<< "data type error!";
			return false;
		}

		if(nullptr == dataPtr) {
			// LOG(ERROR)<< "dataPtr == nullptr!";
			return false;
		}
		// memset(buff, 0, 1024);
		// RreadReturnLen = recv(CsockFd, buff, 1024, 0);
		// if(RreadReturnLen > 0)
		// printf("*************RreadReturnLen = %d\n", RreadReturnLen);
		RreadReturnLen = recv(CsockFd, dataPtr, gRecvLen, 0);
		if (RreadReturnLen < 0){
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR){
				return true;
			}
			// printf("*************exit******************\n");
			return false;
		}else if(0 == RreadReturnLen) {
			// printf("*************设备主动断开连接(通道):\n");                                                                 
			return false;
		}

		if(!RecvSocketFdDataPacket_g()) return false;

	}
	return true;
}


size_t EngineServerJTT::ReadPackLen_g()
{
	size_t RecvLen = 0;
	if(nullptr == RecvRtpPackStr) {
		// LOG(ERROR) << "nullptr == RecvRtpPackStr";
		return false;
	}  

	if(PackStatus_ == _PKG_HD_INIT){
		if(PackHeadLen_ == 0){
			RecvLen = 16;
		}else if(0 < PackHeadLen_ < 16){
			RecvLen = 16 - PackHeadLen_;
		}
		read_packHead_ptr();
	}else if(PackStatus_ == _PKG_HD_REMAINING_INIT_1 && DataType4_ == DATA_TYPE_AUDIO){   //读取头剩余尾部
		if(PackHeadLen_ == 0){
			RecvLen = 8 + 2;
		}else if(0 < PackHeadLen_ < 10){
			RecvLen = 10 - PackHeadLen_;
		}
		read_packHeadAfter_ptr();
	}else if(PackStatus_ == _PKG_HD_REMAINING_INIT_2 && DataType4_ == DATA_TYPE_TRANSM){   
		if(PackHeadLen_ == 0){
			RecvLen = 2;
		}else if(0 < PackHeadLen_ < 2){
			RecvLen = 2 - PackHeadLen_;
		}
		read_packHeadAfter_ptr();
	}else if(PackStatus_ == _PKG_HD_REMAINING_INIT_3 && (DataType4_ == DATA_TYPE_VIDE_I || 
				DataType4_ == DATA_TYPE_VIDE_P || DataType4_ == DATA_TYPE_VIDE_B)){
		if(PackHeadLen_ == 0){
			RecvLen = 8 + 2 + 2 + 2;
		}else if(0 < PackHeadLen_ < 14){
			RecvLen = 14 - PackHeadLen_;
		}
		read_packHeadAfter_ptr();
	}else if(PackStatus_ == _PKG_HD__REMAINING_RECVING && WdBodyLen_ > 0){
		if(PackHeadLen_ == 0){
			RecvLen = WdBodyLen_;
		}else if(0 < PackHeadLen_ < WdBodyLen_){
			RecvLen = WdBodyLen_ - PackHeadLen_;
		}
		gRecvLen = RecvLen;
		read_data_body();
	}else if(PackStatus_ == _PKG_HD_REMAINING_INIT_ERR){
		RecvLen = -1;
	}

	return RecvLen;
}


bool EngineServerJTT::RecvSocketFdDataPacket_g()
{ 
	if(nullptr == RecvRtpPackStr) {
		// LOG(ERROR) << "nullptr == RecvRtpPackStr";
		return false;
	}                                                               
	if(PackStatus_ == _PKG_HD_INIT){
		head_analysis_();
	}else if(PackStatus_ == _PKG_HD_REMAINING_INIT_1){         //0x03 音频
		head_audio_analysis_();
	}else if(PackStatus_ == _PKG_HD_REMAINING_INIT_2){         //0x04 透传
		head_penetrate_analysis_();
	}else if(PackStatus_ == _PKG_HD_REMAINING_INIT_3){         //视频信息
		head_video_analysis_();
	}else if(PackStatus_ == _PKG_HD__REMAINING_RECVING){         //接收报文

		if(DataType4_ != DATA_TYPE_AUDIO && DataType4_ != DATA_TYPE_TRANSM){
			_size += RreadReturnLen;
		}

		PackHeadLen_ += RreadReturnLen;  
		if(PackHeadLen_ == WdBodyLen_){
			if(CREATE_SIM_STATUS_OFF == SIMStatus){
				SIMStatus = CREATE_SIM_STATUS_ON;
				get_device_SIM_();
			}

			if(!videAudioManage()) return false;
			// printf("WdBodyLen_ = %d\n", WdBodyLen_);
			memset(RecvRtpPackStr, 0, sizeof(SAVER_RECV_DATA_SIM6));
			RecvRtpPackStr->PackStatus = _PKG_HD_INIT;
			PackStatus_ = _PKG_HD_INIT;
			PackHeadLen_ = 0;
		}
	}
	return true;
}

bool EngineServerJTT::videAudioManage()
{
	memset(dataStructPtr, 0, sizeof(SEND_VIDEO_INFO_STRU));
	dataStructPtr->Bt8timeStamp = Bt8timeStamp;
	dataStructPtr->timeStamp = _timeStamp;
	channel_ = RecvRtpPackStr->PKG_HEADER.Bt1LogicChannelNumber;
	
	if(DataType4_ != DATA_TYPE_AUDIO && DataType4_ != DATA_TYPE_TRANSM\
			&& (RecvRtpPackStr->PKG_HEADER.subpackageHandleMark4 == 0x00 || RecvRtpPackStr->PKG_HEADER.subpackageHandleMark4 == 0x02)){  
		dataStructPtr->VidePacData = _memptr;
		dataStructPtr->memsize = memsize;
		dataStructPtr->WdBodyLen = _size;
		// printf("********************* %02X\n", DataType4_);
		if(!sendDataV3()) return false;
		memset(_memptr, 0, memsize);
		_size = 0;
	}
	else if(DataType4_ == DATA_TYPE_AUDIO && RecvRtpPackStr->PKG_HEADER.PT7 == LOAD_TYPE_G711A\
			&& (channel_ == 0x04 || channel_ == 0x06 || channel_ == 0x01)){
		if(!push_aduio_data()) return false;
	}
	else if(DataType4_ == DATA_TYPE_AUDIO && RecvRtpPackStr->PKG_HEADER.PT7 == LOAD_TYPE_ADPCMA && channel_ == 0x06){
		if(!push_aduio_data()) return false;
	}
	else if((DataType4_ == DATA_TYPE_TRANSM) || (DataType4_ == DATA_TYPE_AUDIO)){

		memset(audioPtr,0, AUDIO_BUFF_SIZE);
	}

	return true;
}


bool EngineServerJTT::push_aduio_data()
{
	dataStructPtr->VidePacData = audioPtr;
	dataStructPtr->memsize = AUDIO_BUFF_SIZE;
	dataStructPtr->WdBodyLen = WdBodyLen_;
	// if(!send_talk_Audio()) return false;
	if(!send_talk_Audio2()) return false;
	//printf("============channel_ = %02X,RecvRtpPackStr->PKG_HEADER.PT7 = %02X, WdBodyLen_ = %d\n",channel_,RecvRtpPackStr->PKG_HEADER.PT7, WdBodyLen_);
	memset(audioPtr,0, AUDIO_BUFF_SIZE);
	return true;
}

bool EngineServerJTT::send_talk_Audio2()
{
	if(!_sharTalkstrue) return false;

	if(talkStatus == TALK_STATUS_FIST){
		if(!insert_talk_info()) return false;
		talkStatus = TALK_STATUS_END;

		_sharTalkstrue->sharInit(m_BCDSIMStr, dataStructPtr, RecvRtpPackStr->PKG_HEADER.PT7);
	}

	_sharTalkstrue->write_shar_device(/*m_BCDSIMStr, dataStructPtr, RecvRtpPackStr->PKG_HEADER.PT7*/);
	return true;
}

// bool EngineServerJTT::send_talk_Audio()
// {
// 	if(!_talkAudio) return false;

// 	if(talkStatus == TALK_STATUS_FIST){
// 		if(!insert_talk_info()) return false;
// 		if(!(_talkAudio->init(m_BCDSIMStr, dataStructPtr, RecvRtpPackStr->PKG_HEADER.PT7))){
// 			del_audio_type_info(m_BCDSIMStr);
// 			del_audio_connect_info(m_BCDSIMStr);
// 			return false;
// 		}
// 		talkStatus = TALK_STATUS_END;
// 	}

// 	if(!(_talkAudio->run_pust())) return false;
// 	return true;
// }

bool EngineServerJTT::insert_talk_info()
{
	_BYTE audi_type = RecvRtpPackStr->PKG_HEADER.PT7;
	memset(&audioInfo, 0, sizeof(audioType));
	memcpy(audioInfo.BCDSIMCardNumber, RecvRtpPackStr->PKG_HEADER.BCDSIMCardNumber, 6);
	audioInfo.BCDSIMLen = 6;
	audioInfo.ChannelNumber = channel_;
	audioInfo.socketFd = CsockFd;
	if(audi_type == LOAD_TYPE_G711A) {
		audioInfo.Tag_PayloadType = 0x86;
	}else if(audi_type == LOAD_TYPE_ADPCMA){
		audioInfo.Tag_PayloadType = 0x9A;
		if (audioPtr[0] == 0x00 && audioPtr[1] == 0x01 && (audioPtr[2] & 0xff) == (WdBodyLen_ - 4) / 2 && audioPtr[3] == 0x00){
			memcpy(audioInfo.ADPCM_8, audioPtr, 4);
		}
	}else {
		return false;
	}
	
	if(!add_audio_type_info(m_BCDSIMStr,audioInfo)){
		return false;
	}
	return true;
}


void EngineServerJTT::read_data_body()
{
	dataPtr= nullptr;
	if(DataType4_ != DATA_TYPE_AUDIO && DataType4_ != DATA_TYPE_TRANSM){
		if((_size + gRecvLen) > memsize){
			_realptr = nullptr;
			capacity = _size + gRecvLen + 1;
			_realptr = (_BYTE*)realloc(_memptr, capacity);
			_memptr = nullptr;
			_memptr = _realptr;
			memsize = _size + gRecvLen + 1;
		}
		dataPtr = _memptr + _size;
	}else {
		dataPtr = audioPtr + PackHeadLen_;
	}
}

void EngineServerJTT::get_device_SIM_()
{
	memset(m_BCDSIMCard, 0, sizeof(m_BCDSIMCard));
	memcpy(m_BCDSIMCard, RecvRtpPackStr->PKG_HEADER.BCDSIMCardNumber, 6);
	m_BCDSIMStr.clear();
	for(int i = 0; i<6; i++){
		memset(m_TempBCDCard_t, 0, sizeof(m_TempBCDCard_t));
		sprintf( m_TempBCDCard_t, "%02X", m_BCDSIMCard[i] );
		m_BCDSIMStr += m_TempBCDCard_t;
	}

	m_BCDChanStr.clear();
	memset(m_TempBCDCard_t, 0, sizeof(m_TempBCDCard_t));
	sprintf( m_TempBCDCard_t, "%X", RecvRtpPackStr->PKG_HEADER.Bt1LogicChannelNumber);
	m_BCDChanStr += m_TempBCDCard_t;

	StreamName.clear();
	StreamName += m_BCDSIMStr;
	StreamName += '_';
	StreamName += m_BCDChanStr;
}


//解析头
void EngineServerJTT::head_analysis_()
{
	PackHeadLen_ += RreadReturnLen;
	if(PackHeadLen_ == 16){
		AnalyzeHead_16_g(RecvRtpPackStr->HeadPack,  PackHeadLen_);
		if(DataType4_ == 0x03) PackStatus_ = _PKG_HD_REMAINING_INIT_1;
		else if(DataType4_ == 0x04) PackStatus_ = _PKG_HD_REMAINING_INIT_2;
		else if(DataType4_ == 0x00 || DataType4_ == 0x01 || DataType4_ == 0x02) PackStatus_ = _PKG_HD_REMAINING_INIT_3;
		else PackStatus_ = _PKG_HD_REMAINING_INIT_ERR;
		PackHeadLen_ = 0;
	}
}

//解析音频
void EngineServerJTT::head_audio_analysis_()
{
	PackHeadLen_ += RreadReturnLen; 
	if(PackHeadLen_ == 10){
		AnalyzeHeadAudioEnd_10_g(RecvRtpPackStr->HeadAfter);
		PackStatus_ = _PKG_HD__REMAINING_RECVING;
		PackHeadLen_ = 0;
		// add_sharTalkAfterDataType();
	}
}


//解析透传数据
void EngineServerJTT::head_penetrate_analysis_()
{
	PackHeadLen_ += RreadReturnLen;
	if(PackHeadLen_ == 2){
		AnalyzeTransmissionHeadEnd_2_g(RecvRtpPackStr->HeadAfter);
		PackStatus_ = _PKG_HD__REMAINING_RECVING;
		PackHeadLen_ = 0;
	}
}


//解析视频信息
void EngineServerJTT::head_video_analysis_()
{
	PackHeadLen_ += RreadReturnLen;
	if(PackHeadLen_ == 14){
		AnalyzeHeadVideoEnd_14_g(RecvRtpPackStr->HeadAfter);
		PackStatus_ = _PKG_HD__REMAINING_RECVING;
		PackHeadLen_ = 0;
	}
}


bool EngineServerJTT::AnalyzeHead_16_g(const unsigned char* HeadPack16, const int& headLen)
{
	memset(tmpHeadPack16, 0, 21);
	memcpy(tmpHeadPack16, HeadPack16, headLen);
	//printf("----------------pHeader-----------------\n");
	/*for (int i = 0;i<16;++i)
	{
		printf(" %02X",tmpHeadPack16[i]);
	}
	printf("\n");*/

	pTemp = nullptr;
	pTemp = (PACKET_HEAD_SIM6*)tmpHeadPack16;
	RecvRtpPackStr->PKG_HEADER.DWFramHeadMark = pTemp->DWFramHeadMark;
	RecvRtpPackStr->PKG_HEADER.DWFramHeadMark = (RecvRtpPackStr->PKG_HEADER.DWFramHeadMark<<24)|
		(RecvRtpPackStr->PKG_HEADER.DWFramHeadMark&0x0000FF00)<<8|
		(RecvRtpPackStr->PKG_HEADER.DWFramHeadMark&0x00FF0000)>>8|
		(RecvRtpPackStr->PKG_HEADER.DWFramHeadMark)>>24;
	RecvRtpPackStr->PKG_HEADER.V2 = (HeadPack16[4]>>6) & 0x03;
	RecvRtpPackStr->PKG_HEADER.P1 = (HeadPack16[4]>>5) & 0x01;
	RecvRtpPackStr->PKG_HEADER.X1 = (HeadPack16[4]>>4) & 0x01;
	RecvRtpPackStr->PKG_HEADER.CC4 = HeadPack16[4]&0x0f;
	RecvRtpPackStr->PKG_HEADER.M1 = (HeadPack16[5]>>7) & 0x01;
	RecvRtpPackStr->PKG_HEADER.PT7 = HeadPack16[5] & 0x7F;
	RecvRtpPackStr->PKG_HEADER.WdPackageSequence = ntohs(pTemp->WdPackageSequence);
	RecvRtpPackStr->PKG_HEADER.BCDSIMCardNumber[0] = HeadPack16[8];
	RecvRtpPackStr->PKG_HEADER.BCDSIMCardNumber[1] = HeadPack16[9];
	RecvRtpPackStr->PKG_HEADER.BCDSIMCardNumber[2] = HeadPack16[10];
	RecvRtpPackStr->PKG_HEADER.BCDSIMCardNumber[3] = HeadPack16[11];
	RecvRtpPackStr->PKG_HEADER.BCDSIMCardNumber[4] = HeadPack16[12];
	RecvRtpPackStr->PKG_HEADER.BCDSIMCardNumber[5] = HeadPack16[13];
	RecvRtpPackStr->PKG_HEADER.Bt1LogicChannelNumber = HeadPack16[14];
	RecvRtpPackStr->PKG_HEADER.DataType4 = (HeadPack16[15]>>4)&0x0F;
	RecvRtpPackStr->PKG_HEADER.subpackageHandleMark4 = HeadPack16[15]&0x0F;

	// dataType4  = (HeadPack16[15]>>4)&0x0F;
	DataType4_ = (HeadPack16[15]>>4)&0x0F;
	/*	printf("----------------after-----------------\n");
	    printf("DWFramHeadMark = %X\n",RecvRtpPackStr->PKG_HEADER.DWFramHeadMark);
	    printf("V2 = %X\n",RecvRtpPackStr->PKG_HEADER.V2);
	    printf("P1 = %X\n", RecvRtpPackStr->PKG_HEADER.P1);
	    printf("X1 = %X\n",RecvRtpPackStr->PKG_HEADER.X1);
	    printf("CC4 = %X\n",RecvRtpPackStr->PKG_HEADER.CC4);
	    printf("M1 = %X\n",RecvRtpPackStr->PKG_HEADER.M1);
	    printf("PT7 = %X\n",RecvRtpPackStr->PKG_HEADER.PT7);
	    printf("WdPackageSequence = %X\n",RecvRtpPackStr->PKG_HEADER.WdPackageSequence);
	    printf("BCDSIMCardNumber0 = %X\n",RecvRtpPackStr->PKG_HEADER.BCDSIMCardNumber[0]);
	    printf("BCDSIMCardNumber1 = %X\n",RecvRtpPackStr->PKG_HEADER.BCDSIMCardNumber[1]);
	    printf("BCDSIMCardNumber2 = %X\n",RecvRtpPackStr->PKG_HEADER.BCDSIMCardNumber[2]);
	    printf("BCDSIMCardNumber3 = %X\n",RecvRtpPackStr->PKG_HEADER.BCDSIMCardNumber[3]);
	    printf("BCDSIMCardNumber4 = %X\n",RecvRtpPackStr->PKG_HEADER.BCDSIMCardNumber[4]);
	    printf("BCDSIMCardNumber5 = %X\n",RecvRtpPackStr->PKG_HEADER.BCDSIMCardNumber[5]);
	    printf("Bt1LogicChannelNumber = %X\n",RecvRtpPackStr->PKG_HEADER.Bt1LogicChannelNumber);
	    printf("DataType4 = %X\n",RecvRtpPackStr->PKG_HEADER.DataType4);
	    printf("subpackageHandleMark4 = %X\n",RecvRtpPackStr->PKG_HEADER.subpackageHandleMark4);

	    printf("---------------------------------- SIM = %ld\n", RecvRtpPackStr->PKG_HEADER.BCDSIMCardNumber);
	*/
	return true;
}

//数据类型为0x00 0x01 0x02 时解析包头
void EngineServerJTT::AnalyzeHeadVideoEnd_14_g(const unsigned char* HeadPackEnd14)
{
	pTemp2 = nullptr;
	pTemp2 = (PACKET_HEAD_14*)HeadPackEnd14;
	RecvRtpPackStr->PKG_HEADER.Bt8timeStamp = pTemp2->Bt8timeStamp;
	RecvRtpPackStr->PKG_HEADER.Bt8timeStamp = (RecvRtpPackStr->PKG_HEADER.Bt8timeStamp<<56) |
		(RecvRtpPackStr->PKG_HEADER.Bt8timeStamp&0x000000000000FF00)<<40 |
		(RecvRtpPackStr->PKG_HEADER.Bt8timeStamp&0x0000000000FF0000)<<24 |
		(RecvRtpPackStr->PKG_HEADER.Bt8timeStamp&0x00000000FF000000)<<8  |
		(RecvRtpPackStr->PKG_HEADER.Bt8timeStamp&0x000000FF00000000)>>8  |
		(RecvRtpPackStr->PKG_HEADER.Bt8timeStamp&0x0000FF0000000000)>>24 |
		(RecvRtpPackStr->PKG_HEADER.Bt8timeStamp&0x00FF000000000000)>>40 |
		(RecvRtpPackStr->PKG_HEADER.Bt8timeStamp&0xFF00000000000000)>>56;
	RecvRtpPackStr->PKG_HEADER.WdLastIFrameInterval = (HeadPackEnd14[8]<<8)|HeadPackEnd14[9];
	RecvRtpPackStr->PKG_HEADER.WdLastFrameInterval = (HeadPackEnd14[10]<<8)|HeadPackEnd14[11];
	RecvRtpPackStr->PKG_HEADER.WdBodyLen = HeadPackEnd14[12]<<8|HeadPackEnd14[13];

	WdBodyLen_  = 0;
	WdBodyLen_  = HeadPackEnd14[12]<<8|HeadPackEnd14[13];

	_timeStamp = 0;
	Bt8timeStamp = 0;

	Bt8timeStamp = RecvRtpPackStr->PKG_HEADER.Bt8timeStamp;
	_timeStamp = RecvRtpPackStr->PKG_HEADER.WdLastFrameInterval;
	// printf("WdBodyLen = %X\n",RecvRtpPackStr->PKG_HEADER.WdBodyLen);
	// printf("*************** Bt8timeStamp = %ld\n", Bt8timeStamp);
}

//数据类型为0x03时解析包头后10位;   (音频包)
void EngineServerJTT::AnalyzeHeadAudioEnd_10_g(const unsigned char* HeaPaAudioEnd10)
{
	pTemp3 = nullptr;
	pTemp3 = (PACKET_HEAD_10*)HeaPaAudioEnd10;
	RecvRtpPackStr->PKG_HEADER.Bt8timeStamp = pTemp3->Bt8timeStamp;
	RecvRtpPackStr->PKG_HEADER.Bt8timeStamp = (RecvRtpPackStr->PKG_HEADER.Bt8timeStamp<<56) |
		(RecvRtpPackStr->PKG_HEADER.Bt8timeStamp&0x000000000000FF00)<<40 |
		(RecvRtpPackStr->PKG_HEADER.Bt8timeStamp&0x0000000000FF0000)<<24 |
		(RecvRtpPackStr->PKG_HEADER.Bt8timeStamp&0x00000000FF000000)<<8  |
		(RecvRtpPackStr->PKG_HEADER.Bt8timeStamp&0x000000FF00000000)>>8  |
		(RecvRtpPackStr->PKG_HEADER.Bt8timeStamp&0x0000FF0000000000)>>24 |
		(RecvRtpPackStr->PKG_HEADER.Bt8timeStamp&0x00FF000000000000)>>40 |
		(RecvRtpPackStr->PKG_HEADER.Bt8timeStamp&0xFF00000000000000)>>56;
	RecvRtpPackStr->PKG_HEADER.WdBodyLen = HeaPaAudioEnd10[8]<<8|HeaPaAudioEnd10[9];

	WdBodyLen_  = 0;
	WdBodyLen_  = HeaPaAudioEnd10[8]<<8|HeaPaAudioEnd10[9];

	// printf("-------------WdBodyLen_ = %d\n",WdBodyLen_);
	// printf("-------------WdBodyLen2 = %d\n",RecvRtpPackStr->PKG_HEADER.WdBodyLen);
	// printf("*************** Bt8timeStamp = %ld\n", RecvRtpPackStr->PKG_HEADER.Bt8timeStamp);
	// sharTalkdataPtr->WdBodyLen = HeaPaAudioEnd10[8]<<8|HeaPaAudioEnd10[9];
}

//数据类型为0x04时解析包头后10位;   (透传数据)
void EngineServerJTT::AnalyzeTransmissionHeadEnd_2_g(const unsigned char* HeaPaTranEnd2)
{

	RecvRtpPackStr->PKG_HEADER.WdBodyLen = HeaPaTranEnd2[0]<<8|HeaPaTranEnd2[1];

	WdBodyLen_  = 0;
	WdBodyLen_  = HeaPaTranEnd2[0]<<8|HeaPaTranEnd2[1];
}

