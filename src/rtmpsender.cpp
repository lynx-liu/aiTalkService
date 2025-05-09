#include "rtmpsender.h"
#include <unistd.h>
#include <thread>

#define RTMP_HEAD_SIZE   (sizeof(RTMPPacket)+RTMP_MAX_HEADER_SIZE)

RtmpSender::RtmpSender():
spsLen(0),
ppsLen(0),
nTimeStamp(0),
startTime(0)
{
	spsBuff = nullptr;
	ppsBuff = nullptr;
	body = nullptr;
	packet = nullptr;
    spsStatus = SPS_DECODER_STATUS_OF;
    ppsStatus = PPS_DECODER_STATUS_OF;
	m_pRtmp = nullptr;
	packet2 = nullptr;
	gVideoInfoStru = nullptr;
	initStatus = INIT_STATUS_OF;
}

RtmpSender::~RtmpSender()
{
   
}

void RtmpSender::close_free()
{
	 if(nullptr != m_pRtmp){
        RTMP_Close(m_pRtmp);
        RTMP_Free(m_pRtmp);
		m_pRtmp = nullptr;
    }

	if(spsBuff){
		free(spsBuff);
		spsBuff = nullptr;
	}
	if(ppsBuff){
		free(ppsBuff);
		ppsBuff = nullptr;
	}
	if(nullptr != body){
		delete [] body;
		body = nullptr;
	}
	if(nullptr != packet){
		free(packet);
		packet = nullptr;
	}

	spsptr = nullptr;
	ppsptr = nullptr;
	
	if(nullptr != packet2){
		RTMPPacket_Free(packet2);
		free(packet2);
		packet2 = nullptr;
	}
	gVideoInfoStru = nullptr;
}

void RtmpSender::reInit()
{
	if(nullptr != m_pRtmp){
        RTMP_Close(m_pRtmp);
        RTMP_Free(m_pRtmp);
		m_pRtmp = nullptr;
    }

	ppsStatus = PPS_DECODER_STATUS_OF;
	startTime = 0;
}

bool RtmpSender::init(char* rtmpUrl, SEND_VIDEO_INFO_STRU* dataPtr)
{
    m_pRtmp = RTMP_Alloc();
	RTMP_Init(m_pRtmp);
    /*设置URL*/
	if (RTMP_SetupURL(m_pRtmp, rtmpUrl) == FALSE){
        // LOG(ERROR)<< "set URL fail";
		RTMP_Free(m_pRtmp);
		m_pRtmp = nullptr;
		return false;
	}
	RTMP_EnableWrite(m_pRtmp);
	/*连接服务器*/
	if (RTMP_Connect(m_pRtmp, NULL) == FALSE) {
        // LOG(ERROR)<< "connect servicer fail";
		RTMP_Free(m_pRtmp);
		m_pRtmp = nullptr;
		return false;
	} 
	/*连接流*/
	if (RTMP_ConnectStream(m_pRtmp,0) == FALSE){
        // LOG(ERROR)<< "connect stream fail";
		RTMP_Close(m_pRtmp);
		RTMP_Free(m_pRtmp);
		m_pRtmp = nullptr;
		return false;
	}
	
	if(initStatus == INIT_STATUS_OF){
		spsBuff = (Cnvt::uint8_t*)malloc(SPS_PPS_BUFF_SIZE);
		ppsBuff = (Cnvt::uint8_t*)malloc(SPS_PPS_BUFF_SIZE);
		body    = new uint8_t[BODY_SIZE]();

		packet = (RTMPPacket *)malloc(PACKET_BUFF_SIZE);
		packSize = PACKET_BUFF_SIZE;

		packet2 = (RTMPPacket*)malloc(sizeof(RTMPPacket));
		if(!RTMPPacket_Alloc(packet2, PPS_SPS_PACKET_ALLOC_SIZE)){
			// LOG(ERROR)<< "RTMPPacket_Alloc FAIL!";
			close_free();
			return false;
		} 

		gVideoInfoStru = dataPtr;
		initStatus = INIT_STATUS_ON;
	}
	return true;
}

bool RtmpSender::executeProcess(/*SEND_VIDEO_INFO_STRU* gVideoInfoStru*/)
{
	inputBuff = nullptr;
	inputLen = 0;
	Bt8timeStamp = 0;
	_CompositionTime = 0;
	nOffset = 0;

	if(nullptr == gVideoInfoStru){
		// LOG(ERROR) << "nullptr == gVideoInfoStru";
		return false;
	}
	inputBuff = gVideoInfoStru->VidePacData;
	inputLen = gVideoInfoStru->WdBodyLen;
	Bt8timeStamp = gVideoInfoStru->Bt8timeStamp;
	_CompositionTime = gVideoInfoStru->timeStamp;

	for(;;){
		nNaluSize = 0;
		pNalu = nullptr;
		pNalu = inputBuff + nOffset;

		nNaluSize = Cnvt::getNextNalu(pNalu, inputLen - nOffset);
		if(0 == nNaluSize)
			break;
		
		nOffset += nNaluSize;
		if(nNaluSize > 4){
			if(!sendVideoData(/*inputBuff + nOffset, nNaluSize*/)) 
				return false;
		}

		// nOffset += nNaluSize;
		if (nOffset >= inputLen - 4)
			break;
	}
	return true;
}

bool RtmpSender::sendVideoData(/*Cnvt::uint8_t* pNalu, Cnvt::USHORT nNaluSize*/)
{
	naluType = 0;
	nTimeStamp = 0;

	nTimeStamp = (Cnvt::UINT)(Bt8timeStamp - startTime);
	naluType = pNalu[4]&0x1f;
	// printf("********************* %02X\n", pNalu[4]);
	if(0x07 == naluType){ 
		if(!sps(/*pNalu, nNaluSize*/)) return false;
		return true;
	}if(0x08 == naluType){
		if(!pps(/*pNalu, nNaluSize*/)) return false;
		if(ppsStatus == PPS_DECODER_STATUS_OF){
			ppsStatus = PPS_DECODER_STATUS_ON;
			startTime = Bt8timeStamp;
			nTimeStamp = 0;
		}
		if(!SendVideoSpsPps(/*spsBuff, spsLen, ppsBuff, ppsLen, nTimeStamp*/)) return false;
		return true;
	}if(0x06 == naluType){
		return true;
	}if(0x00 == naluType){
		return true;
	}

	if(!sendH264Frame(/* pNalu, nNaluSize, naluType, nTimeStamp*/)) return false;
	return true;
}

bool RtmpSender::sps(/*Cnvt::uint8_t* pNalu, Cnvt::USHORT nNaluSize&*/)
{
	if(nNaluSize > SPS_PPS_BUFF_SIZE){
		// LOG(ERROR) << "sps nNaluSize > "<< SPS_PPS_BUFF_SIZE;
		return false; 
	}
	spsLen = 0;
	memset(spsBuff, 0, SPS_PPS_BUFF_SIZE);
	memcpy(spsBuff, pNalu, nNaluSize);
	spsLen = nNaluSize;
	return true;
}
bool RtmpSender::pps(/*Cnvt::uint8_t* pNalu, Cnvt::USHORT nNaluSize*/)
{
	if(nNaluSize > SPS_PPS_BUFF_SIZE){
		// LOG(ERROR) << "pps nNaluSize > "<< SPS_PPS_BUFF_SIZE;
		return false; 
	}
	ppsLen = 0;
	memset(ppsBuff, 0, SPS_PPS_BUFF_SIZE);
	memcpy(ppsBuff, pNalu, nNaluSize);
	ppsLen = nNaluSize;
	return true;
}

// int body_size = 16 + sps_len + pps_len; //按照H264标准配置SPS和PPS，共使用了16字节
int RtmpSender::SendVideoSpsPps(/*Cnvt::uint8_t* spsptr,int sps_len,Cnvt::uint8_t* ppsptr,int pps_len, Cnvt::UINT _TimeStamp*/)
{
	spsptr = nullptr;
	ppsptr = nullptr;
	sps_len = 0;
	pps_len = 0;

	spsptr = spsBuff + 4;
	sps_len = spsLen - 4;
	ppsptr = ppsBuff + 4;
	pps_len = ppsLen - 4;
	if(spsptr == nullptr || sps_len <= 0) return 0;
	if(ppsptr == nullptr || pps_len <= 0) return 0;

	RTMPPacket_Reset(packet2);
	memset(packet2->m_body - RTMP_MAX_HEADER_SIZE, 0, RTMP_MAX_HEADER_SIZE + PPS_SPS_PACKET_ALLOC_SIZE);
	char* body = packet2->m_body;

	// memset(body, 0, BODY_SIZE);
	int i = 0;
	body[i++] = 0x17;
	body[i++] = 0x00;

	body[i++] = 0x00;
	body[i++] = 0x00;
	body[i++] = 0x00;

	/*AVCDecoderConfigurationRecord*/
	body[i++] = 0x01;
	body[i++] = spsptr[1];
	body[i++] = spsptr[2];
	body[i++] = spsptr[3];
	body[i++] = 0xff;

	/*sps*/
	body[i++]   = 0xe1;
	body[i++] = (sps_len >> 8) & 0xff;
	body[i++] = sps_len & 0xff;
	// memcpy(&body[i],spsptr,sps_len);
	memcpy(body+i,spsptr,sps_len);
	i +=  sps_len;

	/*pps*/
	body[i++]   = 0x01;
	body[i++] = (pps_len >> 8) & 0xff;
	body[i++] = (pps_len) & 0xff;
	// memcpy(&body[i],ppsptr,pps_len);
	memcpy(body+i,ppsptr,pps_len);
	i +=  pps_len;

	packet2->m_packetType = RTMP_PACKET_TYPE_VIDEO;
	packet2->m_nBodySize = i;
	packet2->m_nChannel = 0x04;
	packet2->m_nTimeStamp = 0;
	packet2->m_hasAbsTimestamp = 0;
	packet2->m_headerType = RTMP_PACKET_SIZE_MEDIUM; 
	packet2->m_nInfoField2 = m_pRtmp->m_stream_id;

	if(nullptr == m_pRtmp){
		// LOG(ERROR)<< "nullptr == m_pRtmp";
		return 0;
	} 
	if (!RTMP_IsConnected(m_pRtmp)){
		// RTMPPacket_Free(&packet);
		return 0;
	}
	int nRet = RTMP_SendPacket(m_pRtmp,packet2,TRUE);
	// RTMPPacket_Free(&packet);
	return nRet;
}


int RtmpSender::sendH264Frame(/*Cnvt::uint8_t* dataptr, int len, Cnvt::UINT naluType, Cnvt::UINT nTimeStamp*/)
{
	pNalu += 4;
	nNaluSize -= 4;

	// uint8_t body[MAX_BUFF_SIZE];
	memset(body, 0, BODY_SIZE);
	int i = 0;
	if(naluType == 0x05)
		body[i++] = 0x17;
	else
		body[i++] = 0x27;
	//AVC NALU
	body[i++] = 0x01;
	body[i++] = 0x00;
	body[i++] = 0x00;
	body[i++] = 0x00;
	//NALU SIZE
	body[i++] = nNaluSize >> 24 &0xff;
	body[i++] = nNaluSize >> 16 &0xff;
	body[i++] = nNaluSize >> 8 &0xff;
	body[i++] = nNaluSize &0xff;
	//NALU

	if((RTMP_HEAD_SIZE+nNaluSize+i) > packSize){
		RTMPPacket* _packet = (RTMPPacket*)realloc(packet, RTMP_HEAD_SIZE+nNaluSize+i);
		packet = nullptr;
		packet = _packet;
		packSize = RTMP_HEAD_SIZE+nNaluSize+i;
	}
	memset(packet,0,packSize);

	packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
	packet->m_nBodySize = nNaluSize+i;
	memcpy(packet->m_body, body, i);
	memcpy(packet->m_body+i, pNalu, nNaluSize);
	packet->m_hasAbsTimestamp = 0;
	packet->m_packetType = RTMP_PACKET_TYPE_VIDEO; 
	packet->m_nInfoField2 = m_pRtmp->m_stream_id;
	packet->m_nChannel = 0x04;

	packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
	packet->m_nTimeStamp = nTimeStamp;

	int nRet =0;
	if(nullptr == m_pRtmp){
		// LOG(ERROR)<< "nullptr == m_pRtmp";
		return 0;
	} 
	if (RTMP_IsConnected(m_pRtmp)){
		nRet = RTMP_SendPacket(m_pRtmp,packet,TRUE);
	}
	// free(packet);
	return nRet;  
}



int RtmpSender::SendPacket(Cnvt::UINT packType,Cnvt::uint8_t*data,Cnvt::UINT len, Cnvt::UINT timestamp)  
{  
	RTMPPacket* packet;
	packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE+len);
	memset(packet,0,RTMP_HEAD_SIZE);
	packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
	packet->m_nBodySize = len;
	memcpy(packet->m_body,data,len);
	packet->m_hasAbsTimestamp = 0;
	packet->m_packetType = packType; /*此处为类型有两种一种是音频,一种是视频*/
	packet->m_nInfoField2 = m_pRtmp->m_stream_id;
	packet->m_nChannel = 0x04;

	packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
	if (RTMP_PACKET_TYPE_AUDIO ==packType && len !=4)
	{
		packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
	}
	packet->m_nTimeStamp = timestamp;

	int nRet =0;
	if (RTMP_IsConnected(m_pRtmp)){
		nRet = RTMP_SendPacket(m_pRtmp,packet,TRUE); /*TRUE为放进发送队列,FALSE是不放进发送队列,直接发送*/
	}
	free(packet);
	return nRet;  
}  

