#include <cstring>
#include <unistd.h>
#include <memory>
#include "RTPServerEngine.h"

CRTPServerEngine::CRTPServerEngine(const int fd, const CONFIG ServerConfig)
{
	sockFd = fd;
	_sharTalkstrue = new SharTalkAudio(ServerConfig);
}

CRTPServerEngine::~CRTPServerEngine()
{
	close(sockFd);

	if(_sharTalkstrue){
		delete _sharTalkstrue;
		_sharTalkstrue = nullptr;
	}
}

void CRTPServerEngine::reInit()
{
	close(sockFd);

	if (_sharTalkstrue) _sharTalkstrue->reint();
	if(!m_BCDSIMStr.empty()){
		del_audio_type_info(m_BCDSIMStr);
		m_BCDSIMStr.clear();
	}
}

bool CRTPServerEngine::ReadAndAnalyzeRTPPack()
{
	uint8_t bcdLen = 6;
	RTP_PKG_HEADER header;
	std::unique_ptr<uint8_t[]> data(new uint8_t[AUDIO_BUFF_SIZE]);
	size_t nFixHeadSize = offsetof(RTP_PKG_HEADER, BCDSIMCardNumber) + 6;
	
	while(true) {
		if(recv(sockFd, &header, nFixHeadSize, MSG_WAITALL)<=0)
			return false;
		
		if(header.BCDSIMCardNumber[0]==0x00 && header.BCDSIMCardNumber[1] == 0x00 && header.BCDSIMCardNumber[2] == 0x00 && header.BCDSIMCardNumber[3]  == 0x00) {
			bcdLen = 10;
			if(recv(sockFd, (uint8_t*)header.BCDSIMCardNumber + 6, sizeof(RTP_PKG_HEADER)-nFixHeadSize, MSG_WAITALL)<=0)
				return false;
		} else {
			bcdLen = 6;
			if(recv(sockFd, &header.Bt1LogicChannelNumber, sizeof(RTP_PKG_HEADER) - offsetof(RTP_PKG_HEADER, Bt1LogicChannelNumber), MSG_WAITALL)<=0)
				return false;
		}

		header.WdBodyLen = ntohs(header.WdBodyLen);/*
		header.WdPackageSequence = ntohs(header.WdPackageSequence);
		header.Bt8timeStamp = ntohll(header.Bt8timeStamp);
		header.DWFramHeadMark = ntohl(header.DWFramHeadMark);*/

		if(m_BCDSIMStr.empty()) {
			get_device_SIM(header.BCDSIMCardNumber, bcdLen);

			if(!insert_talk_info(data.get(), header, bcdLen))
				return false;
						
			if(!_sharTalkstrue->sharInit(m_BCDSIMStr, header.type))
				return false;
		}

		if(recv(sockFd, data.get(), header.WdBodyLen, MSG_WAITALL)<=0)
			return false;
		_sharTalkstrue->write_shar_device(data.get(), header.WdBodyLen);
	}
	return true;
}

bool CRTPServerEngine::insert_talk_info(const uint8_t* data, RTP_PKG_HEADER &header, uint8_t bcdLen)
{
	audioType audioInfo;
	memset(&audioInfo, 0, sizeof(audioType));
	memcpy(audioInfo.BCDSIMCardNumber, header.BCDSIMCardNumber, bcdLen);
	audioInfo.BCDSIMLen = bcdLen;
	audioInfo.ChannelNumber = header.Bt1LogicChannelNumber;
	audioInfo.socketFd = sockFd;
	audioInfo.type = header.type;

	if(audioInfo.type&0x7F == LOAD_TYPE_ADPCM){
		if (data[0] == 0x00 && data[1] == 0x01 && data[2] == (header.WdBodyLen - 4) / 2 && data[3] == 0x00){
			memcpy(audioInfo.ADPCM_8, data, 4);
		}
	}
	
	return add_audio_type_info(m_BCDSIMStr,audioInfo);
}

void CRTPServerEngine::get_device_SIM(uint8_t* bcdSim, uint8_t bcdLen)
{
	char temp[3] = {0};
	m_BCDSIMStr.clear();
	for(int i = 0; i<bcdLen; i++){
		sprintf(temp, "%02X", bcdSim[i]);
		m_BCDSIMStr += temp;
	}
}