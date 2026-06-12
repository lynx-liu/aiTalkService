#include <cstring>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <memory>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <openssl/md5.h>
#include "debug.h"
#include "tiny_ws.h"
#include "RTPServerEngine.h"

#define VIDEO_FRAME_SIZE (32 * BUFF_SIZE)

CRTPServerEngine::CRTPServerEngine(const int fd, const CONFIG ServerConfig)
{
	sockFd = fd;
	_sharTalkstrue = std::make_shared<SharTalkAudio>(ServerConfig);
}

CRTPServerEngine::~CRTPServerEngine()
{
	close(sockFd);

	if (_sharTalkstrue) {
		_sharTalkstrue->reint();
	}

	if(!m_BCDSIMStr.empty()){
		printf("\n%sremoved device SIM: %s\n", getNowTime().data(), m_BCDSIMStr.c_str());
		del_audio_type_info(m_BCDSIMStr);
		m_BCDSIMStr.clear();
	}

	_sharTalkstrue.reset();
}

void CRTPServerEngine::ReadAndAnalyzeRTPPack()
{
	uint8_t bcdLen = 0;
	RTP_PKG_HEADER header;
	std::unique_ptr<uint8_t[]> data(new uint8_t[BUFF_SIZE]);
	size_t nFixHeadSize = offsetof(RTP_PKG_HEADER, Bt8timeStamp);
	
	std::shared_ptr<RtmpSender> m_RtmpSender;
	std::vector<uint8_t> videoFrameBuf;

	while(true) {
		memset(&header, 0, sizeof(RTP_PKG_HEADER));
		int ret = recv(sockFd, &header, nFixHeadSize, MSG_WAITALL);
		if(ret==0) break;
		
		if(ret < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR || errno == ECONNRESET) {
				continue;
			}
			printf("\n%sFailed to receive RTP header, fd=%d, err:%d", getNowTime().data(), sockFd, errno);
			break;
		}

		header.DWFramHeadMark = ntohl(header.DWFramHeadMark);
		if(header.DWFramHeadMark != 0x30316364) {
			printf("\n%sInvalid frame head mark: 0x%08X", getNowTime().data(), header.DWFramHeadMark);
			break;
		}
		
		int DataType = (header.info >> 4) & 0x0F;
		int subpackageHandleMark = header.info & 0x0F;

		if(bcdLen <= 0)
			bcdLen = getBcdLen(header.BCDSIMCardNumber, DataType, subpackageHandleMark);

		int offset = 0;
		if(bcdLen < 10) {
			offset = 10 - bcdLen;
			// 内存修正：将 Bt1LogicChannelNumber 及后续字段前移
        	memmove(&header.Bt1LogicChannelNumber, header.BCDSIMCardNumber + bcdLen, 
				nFixHeadSize - offsetof(RTP_PKG_HEADER, Bt1LogicChannelNumber) + offset);
			// 清零多余的 SIM 号部分
			memset(header.BCDSIMCardNumber + bcdLen, 0, offset);

			DataType = (header.info >> 4) & 0x0F;
			subpackageHandleMark = header.info & 0x0F;
		}

		if(DataType >= DATA_TYPE_TRANSM || subpackageHandleMark>PKG_FLAG_MIDDLE) {//透传数据不处理,包标错误不处理
			printf("\n%sUnsupported DataType:0x%02X or subpackageHandleMark: %0d", getNowTime().data(), DataType, subpackageHandleMark);
			break;
		}

		if(recv(sockFd, (uint8_t*)&header.Bt8timeStamp+offset, sizeof(header.Bt8timeStamp)-offset, MSG_WAITALL)<=0) {
			printf("\n%sFailed to receive RTP timestamp", getNowTime().data());
			break;
		}

		if(DataType < DATA_TYPE_AUDIO) {//视频 Last IFrame Interval 与 Last Frame Interval
			uint32_t videoTimestamp = 0;
			if (recv(sockFd, &videoTimestamp, sizeof(videoTimestamp), MSG_WAITALL) <= 0) {
				printf("\n%sFailed to receive video timestamp", getNowTime().data());
				break;
			}
		}
		
		if(recv(sockFd, &header.WdBodyLen, sizeof(header.WdBodyLen), MSG_WAITALL)<=0) {
			printf("\n%sFailed to receive RTP body length", getNowTime().data());
			break;
		}

		header.WdBodyLen = ntohs(header.WdBodyLen);
		header.WdPackageSequence = ntohs(header.WdPackageSequence);
		header.Bt8timeStamp = ntohll(header.Bt8timeStamp);

		if(header.WdBodyLen > BUFF_SIZE) {
			printf("\n%sBody length %d exceeds buffer size", getNowTime().data(), header.WdBodyLen);
			break;
		}

		if(m_BCDSIMStr.empty()) {
			get_device_SIM(header.BCDSIMCardNumber, bcdLen);

			if(DataType == DATA_TYPE_AUDIO) {
				if(!insert_talk_info(data.get(), header, bcdLen))
					break;
							
				if(!_sharTalkstrue->sharInit(m_BCDSIMStr, header.type))
					break;
			} else {
				if(videoFrameBuf.empty())
					videoFrameBuf.reserve(VIDEO_FRAME_SIZE);
				videoFrameBuf.clear();

				std::string liveName = m_BCDSIMStr + "_" + std::to_string(header.Bt1LogicChannelNumber);
				std::string rtmpUrl = "rtmp://192.168.0.85:1935/live/"+liveName;
				if(bcdLen == 10) {
					std::string key = "2bd8c611ec3498d791595df39669a904";
					std::string timeHexStr = getPushTimeHexString();//获取有效时间戳
					std::string secret = getMD5(key + liveName + timeHexStr);

					rtmpUrl = "rtmp://test.livepush.che-mi.net/live/"+liveName+"?txSecret="+secret+"&txTime="+timeHexStr;
				} else {
					rtmpUrl = "rtmp://127.0.0.1:3935/live/"+liveName;// 本机公网IP:112.74.99.117
				}
				printf("\n%sRTMP URL: %s", getNowTime().data(), rtmpUrl.c_str());

				m_RtmpSender = std::make_shared<RtmpSender>();
				if (!m_RtmpSender->Init(rtmpUrl)) {
					m_RtmpSender.reset();
					printf("\n%sFailed to initialize RTMP sender", getNowTime().data());
					break;
				}
			}
		}

#if DEBUG
		switch(subpackageHandleMark) {
			case PKG_FLAG_ATOM:
				printf("\n%sType:0x%02X, Len:%d, Channel: %d, timeStamp: %ld", getNowTime().data(), DataType, header.WdBodyLen, header.Bt1LogicChannelNumber, header.Bt8timeStamp);
				break;
			case PKG_FLAG_FIRST:
				printf("\n%sType:0x%02X, Len:%d, Channel: %d, timeStamp: %ld,", getNowTime().data(), DataType, header.WdBodyLen, header.Bt1LogicChannelNumber, header.Bt8timeStamp);
				break;
			case PKG_FLAG_LAST:
				printf("\n");
				break;
			case PKG_FLAG_MIDDLE:
				printf(" -");
				break;
		}
#endif

		if(recv(sockFd, data.get(), header.WdBodyLen, MSG_WAITALL)<=0) {
			printf("\n%sFailed to receive RTP body", getNowTime().data());
			break;
		}

		if(m_RtmpSender) {
			if(DataType < DATA_TYPE_AUDIO) {
				if(subpackageHandleMark == PKG_FLAG_ATOM || subpackageHandleMark == PKG_FLAG_FIRST) {
					videoFrameBuf.clear();
				}

				if(videoFrameBuf.size() + header.WdBodyLen > videoFrameBuf.capacity()) {
					printf("\n%sreserve Video frame buffer", getNowTime().data());
					videoFrameBuf.reserve(videoFrameBuf.capacity() + VIDEO_FRAME_SIZE);
				}

				videoFrameBuf.insert(videoFrameBuf.end(), data.get(), data.get() + header.WdBodyLen);

				if(subpackageHandleMark == PKG_FLAG_ATOM || subpackageHandleMark == PKG_FLAG_LAST) {
					bool isKeyFrame = (DataType == DATA_TYPE_VIDE_I);
					if (!m_RtmpSender->SendH264Frame(videoFrameBuf.data(), videoFrameBuf.size(), header.Bt8timeStamp, isKeyFrame)) {
						printf("\n%sFailed to send H264 frame", getNowTime().data());
						break;
					}

					videoFrameBuf.clear();
				}
			}
		} else {
			if(DataType == DATA_TYPE_AUDIO) {
				if(!_sharTalkstrue->write_shar_device(data.get(), header.WdBodyLen)) {
					printf("\n%sFailed to write audio data to device", getNowTime().data());
					break;
				}
			}
		}
	}

	if(m_RtmpSender) {
		m_RtmpSender->Close();
		m_RtmpSender.reset();
	}
}

//获取十六进制推流有效时间 (如:5DD435ED) 默认当前时间+12小时
std::string CRTPServerEngine::getPushTimeHexString()
{
    std::ostringstream oss;
    oss << std::uppercase << std::hex << static_cast<unsigned long>( time(nullptr) + 60 * 60 * 12 );
    return oss.str();
}

 // 函数说明:防盗链KEYmd5加密
std::string CRTPServerEngine::getMD5(const std::string& str)
{
    unsigned char md[16] = {0};
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, reinterpret_cast<const unsigned char*>(str.data()), str.size());
    MD5_Final(md, &ctx);

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (int i = 0; i < sizeof(md); ++i)
        oss << std::setw(2) << static_cast<int>(md[i]);

    return oss.str();
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

	// Bound blocking send() on the device TCP connection.
	// Prevents long stalls when the peer is slow/unresponsive.
	{
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 500 * 1000;
		if (setsockopt(sockFd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) != 0) {
			perror("setsockopt SO_SNDTIMEO");
		}
	}

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

int CRTPServerEngine::getBcdLen(uint8_t* bcdSim, int DataType, int subpackageHandleMark)
{
	if(bcdSim[0]==0x00 && bcdSim[1] == 0x00 && bcdSim[2] == 0x00 && bcdSim[3]  == 0x00
		&& DataType <= DATA_TYPE_TRANSM && subpackageHandleMark <= PKG_FLAG_MIDDLE)
		return 10;
	return 6;
}
