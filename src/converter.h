#ifndef CONVERTER_H
#define CONVERTER_H

#include <fstream>
#include <cstring>
#include <list>
#include "splitter.h"
#include "lock.h"
#include <map>
#include "StreDataType.h"

#define MAX_BUFF_SIZE_ 1024
#define SEND_SPS_PPS_BUFF_SIZE 1024
#define SEND_SPS_PPS_TO_FD_NON 0
#define SEND_SPS_PPS_TO_FD_YES 1
#define GET_SPS_PPS_NON 2
#define GET_SPS_PPS_YES 3
#define FIRST_NALU_TYPE_NON_SPS 4
#define FIRST_NALU_TYPE_YES_SPS 5

namespace Cnvt
{
	class u4
	{
	public:
		u4(unsigned int i) { _u[0] = i >> 24; _u[1] = (i >> 16) & 0xff; _u[2] = (i >> 8) & 0xff; _u[3] = i & 0xff; }

	public:
		unsigned char _u[4];
	};
	class u3
	{
	public:
		u3(unsigned int i) { _u[0] = i >> 16; _u[1] = (i >> 8) & 0xff; _u[2] = i & 0xff; }

	public:
		unsigned char _u[3];
	};
	class u2
	{
	public:
		u2(unsigned int i) { _u[0] = i >> 8; _u[1] = i & 0xff; }

	public:
		unsigned char _u[2];
	};

	typedef struct fdInfoStruct{
		UINT num;
		ULONG startTime;
		int  _PrevTagSize;
	}Fd_INFO;

	class CConverter
	{
	public:
		CConverter();
		virtual ~CConverter();

		int ConvertAAC(char *pAAC, int nAACFrameSize, unsigned int nTimeStamp);
		bool executeProcess(SEND_VIDEO_INFO_STRU* gVideoInfoStru); //uint8_t* inputBuff, unsigned short inputLen
		void init(int _deviceFd, std::string sim_c, CConverter* cnvtOBJ);
		void AddOneNewFd(int fd);
		void CloseFd();

	private:
		void MakeFlvHeader(unsigned char *pFlvHeader);

		// h.264
		int ScriptTag();
		int WriteH264Header(unsigned int _TimeStamp, int fd, int _nPrevTagSize1);
		int WriteH264Frame(uint8_t* pNalu, USHORT nNaluSize, unsigned int nTimeStamp, int fd, int PrevTagSize, UINT CompositionTime);
		void WriteH264EndofSeq(int fd);

		// aac
		void WriteAACHeader(unsigned int nTimeStamp);
		void WriteAACFrame(char *pFrame, int nFrameSize, unsigned int nTimeStamp);

		void Write(unsigned char u) { _fileOut.write((char *)&u, 1); }
		void Write(u4 u) { _fileOut.write((char *)u._u, 4); }
		void Write(u3 u) { _fileOut.write((char *)u._u, 3); }
		void Write(u2 u) { _fileOut.write((char *)u._u, 2); }

		//request fd
		int  getListSize();
		bool sendFLVhead(int fd);		
		bool sendSpsPPS(uint8_t* pNalu, USHORT nNaluSize);
		bool sendVideoData(uint8_t* pNalu, USHORT nNaluSize);
		void copyFd();

		void sps(uint8_t* pNalu, USHORT nNaluSize);
		void pps(uint8_t* pNalu, USHORT nNaluSize);

	private:
		uint8_t _FlvHeader[9];
		uint8_t* _pSPS;
		uint8_t* _pPPS;
		int _nSPSSize;
		int _nPPSSize;
		int _bWriteAVCSeqHeader;
		int _nPrevTagSize;
		int _nStreamID;
		int _nVideoTimeStamp;

		uint8_t* _pAudioSpecificConfig;
		int _nAudioConfigSize;
		int _aacProfile;
		int _sampleRateIndex;
		int _channelConfig;
		int _bWriteAACSeqHeader;

	private:
		std::fstream _fileOut;

	private:
		int _bHaveAudio, _bHaveVideo;
		unsigned int   nTimeStamp;
		unsigned char* g_pBufferOut;

		int rePrevTagSize;
		int _PrevTagSize;
		UINT _num;
		UINT timeStamp;
		Fd_INFO fdinfo;

	private:
		int	  width; 
		int	  height;
		int   fps;
		int	  frameRate;  
		int   _status;
		int   connNum;
		int   listSize;
		int   firstNun;
		int   naluType;
		int   spsStatus;
		int	  deviceFd;
		UINT timeTick; 
		uint8_t* body;
		uint8_t* inputBuff;
		USHORT inputLen;
		ULONG  Bt8timeStamp;
		ULONG  _startTime;
		USHORT _CompositionTime;
		Mutex matex;
		std::list<int> requFdList;
		std::list<int>::iterator iter;

		std::list<int> I_List;
		std::list<int>::iterator I_iter;

		std::list<int> _fdList;
		std::list<int>::iterator _iter;

		std::map<int, Fd_INFO> fdInfoMap;
		std::map<int, Fd_INFO>::iterator itermap;

		std::string _sim_c;

		FILE* file;

	};

}
extern int get_http_quest_info_h(std::string sim_c);
extern bool input_info(std::string sim_c, Cnvt::CConverter* _cnvtOBJ);
extern void delete_input_info(std::string sim_c);
#endif // CONVERTER_H
