#include <iostream>
#include "converter.h"

using namespace std;
namespace Cnvt
{
	CConverter::CConverter():nTimeStamp(0),connNum(0),listSize(0),
	width(0),
	height(0),
	fps(0),
	frameRate(0),
	timeTick(0),
	firstNun(0),
	deviceFd(-1)
	{
		_pSPS = nullptr;
		_pPPS = nullptr;
		_nSPSSize = 0;
		_nPPSSize = 0;
		_bWriteAVCSeqHeader = 0;
		_nPrevTagSize = 0;
		_nStreamID = 0;

		_pAudioSpecificConfig = nullptr;
		_nAudioConfigSize = 0;
		_aacProfile = 0;
		_sampleRateIndex = 0;
		_channelConfig = 0;
		_bWriteAACSeqHeader = 0;

	}

	CConverter::~CConverter()
	{
		if(nullptr != body){
			delete[] body;
		}
		if (nullptr != _pSPS)
			delete[] _pSPS;
		if (nullptr != _pPPS)
			delete[] _pPPS;
		// if(nullptr != g_pBufferOut)
		// 	delete[] g_pBufferOut;
		CloseFd();
	}


	void CConverter::CloseFd()
	{
		delete_input_info(_sim_c);

		_iter = _fdList.begin();
		while (_iter != _fdList.end()){
			close(*_iter);
			_fdList.erase(_iter++);
		}

		I_iter = I_List.begin();
		while (I_iter != I_List.end()){
			close(*I_iter);
			I_List.erase(I_iter++);
		}

		matex.mutex_lock();
		iter = requFdList.begin();
		while (iter != requFdList.end()){
			close(*iter);
			_fdList.erase(iter++);
		}
		matex.mutex_unlock();
	}


	void CConverter::init(int _deviceFd, std::string sim_c, CConverter* cnvtOBJ)
	{
		input_info(sim_c, cnvtOBJ);
		_sim_c = sim_c;
		body  = new uint8_t[MAX_BUFF_SIZE_]();
		_pSPS = new uint8_t[SEND_SPS_PPS_BUFF_SIZE]();
		_pPPS = new uint8_t[SEND_SPS_PPS_BUFF_SIZE]();
		// g_pBufferOut = new unsigned char[MAX_BUFF_SIZE_]();
		_bHaveVideo = 1;
		_status = GET_SPS_PPS_NON;
		spsStatus = FIRST_NALU_TYPE_NON_SPS;
		deviceFd = _deviceFd;
		MakeFlvHeader(_FlvHeader);

		// file = fopen("test.flv","w");
	}


	void CConverter::AddOneNewFd(int fd)
	{
		matex.mutex_lock();
		requFdList.push_back(fd);
		listSize++;
		matex.mutex_unlock();
	}

	int CConverter::getListSize()
	{
		int _listSize = 0;
		matex.mutex_lock();
		_listSize = listSize;
		matex.mutex_unlock();
		return _listSize;
	}

	void CConverter::sps(uint8_t* pNalu, USHORT nNaluSize)
	{
		_nSPSSize = 0;
		memset(_pSPS, 0, SEND_SPS_PPS_BUFF_SIZE);
		memcpy(_pSPS, pNalu, nNaluSize);
		_nSPSSize = nNaluSize;
	}

	void CConverter::pps(uint8_t* pNalu, USHORT nNaluSize)
	{
		_nPPSSize = 0;
		memset(_pPPS, 0, SEND_SPS_PPS_BUFF_SIZE);
		memcpy(_pPPS, pNalu, nNaluSize);
		_nPPSSize = nNaluSize;
	}

	void CConverter::copyFd()
	{
		int fd = -1;
		I_iter = I_List.begin();
		while (I_iter != I_List.end()){
			fd = (*I_iter);
			I_List.erase(I_iter++);
			firstNun--;
			_fdList.push_back(fd);
			connNum++;
		}
	}

	bool CConverter::sendSpsPPS(uint8_t* pNalu, USHORT nNaluSize)
	{
		int count = 0;
		int _fd = -1;
		int PrevTagSize = 0;
		int _PrevTagSize = 0;
		UINT _num = 0;
		UINT timeStamp = 0;
		Fd_INFO fdinfo = {};
		
		matex.mutex_lock();
		iter = requFdList.begin();
		while (iter != requFdList.end()){
			_fd = (*iter);
			requFdList.erase(iter++);
			listSize--;

			_num         = 0;
			timeStamp    = 0;
			PrevTagSize  = 0;
			_PrevTagSize = 0;
			if(!sendFLVhead(_fd)){
				close(_fd);
				continue;
			}
			PrevTagSize = WriteH264Header(0, _fd, 0); //(UINT)Bt8timeStamp
			if(!PrevTagSize){
				close(_fd);
				continue;
			}
			// _num++;
			// timeStamp = _num*timeTick;
			_PrevTagSize = WriteH264Frame(pNalu, nNaluSize, 0, _fd, PrevTagSize, _CompositionTime);
			if(!_PrevTagSize){
				close(_fd);
				continue;
			}else{
				I_List.push_back(_fd);
				firstNun++;

				// _num++;
				fdinfo.num = _num;
				fdinfo.startTime = Bt8timeStamp;
				fdinfo._PrevTagSize = _PrevTagSize;
				fdInfoMap[_fd] = fdinfo;
			}
		}
		matex.mutex_unlock();

		return true;
	}

	bool CConverter::sendVideoData(uint8_t* pNalu, USHORT nNaluSize)
	{
		naluType = 0;
		naluType = pNalu[4]&0x1f;
		printf("********************* %02x\n", pNalu[4]);

		// if(spsStatus == FIRST_NALU_TYPE_NON_SPS){
		// 	if(naluType != 0x07){
		// 		close(deviceFd);
		// 		return false;
		// 	}else {
		// 		spsStatus = FIRST_NALU_TYPE_YES_SPS;
		// 	}
		// }

		if(0x07 == naluType){ 
			sps(pNalu, nNaluSize);
			return true;
		}if(0x08 == naluType){
			pps(pNalu, nNaluSize);
			return true;
		}if(0x06 == naluType){
			return true;
		}if(0x00 == naluType){
			return true;
		}


		if((getListSize() > 0) && (0x05 == naluType)){
			sendSpsPPS(pNalu, nNaluSize);
		}

		if((firstNun > 0) && (0x01 == naluType)){
			copyFd();
		}

		if(connNum <= 0) 
			return false;
		
		int fd;
		_iter = _fdList.begin();
		while (_iter != _fdList.end()){
			fd = -1;
			fd = (*_iter);

			_num = 0;
			timeStamp = 0;
			_startTime = 0;
			memset(&fdinfo, 0, sizeof(Fd_INFO));
			itermap = fdInfoMap.find(fd);
			if(itermap != fdInfoMap.end()){
				fdinfo = fdInfoMap[fd];
				_startTime = fdinfo.startTime;
			}else{
				close(fd);
				_fdList.erase(_iter++);
				connNum--;
				continue;
			}
			
			rePrevTagSize = 0;
			_PrevTagSize = 0;
			timeStamp = (UINT)(Bt8timeStamp - _startTime);
			if(0x05 == naluType){
				// _num++;
				// timeStamp = timeTick * _num;
				rePrevTagSize = WriteH264Header(timeStamp, fd, fdinfo._PrevTagSize);
				if(!rePrevTagSize){
					close(fd);
					_fdList.erase(_iter++);
					connNum--;
					fdInfoMap.erase(itermap);
					continue;
				}
				fdinfo._PrevTagSize = rePrevTagSize;
			}
			
			// _num++;
			// timeStamp = timeTick * _num;
			_PrevTagSize = WriteH264Frame(pNalu, nNaluSize, timeStamp, fd, fdinfo._PrevTagSize, _CompositionTime);
			if(!_PrevTagSize){
				close(fd);
				_fdList.erase(_iter++);
				connNum--;
				fdInfoMap.erase(itermap);
				continue;
			}
			fdinfo.num = _num;
			fdinfo.startTime = _startTime;
			fdinfo._PrevTagSize = _PrevTagSize;
			fdInfoMap[fd] = fdinfo;
			_iter++;
		}
		return true;
	}

	bool CConverter::executeProcess(SEND_VIDEO_INFO_STRU* gVideoInfoStru)
	{
		inputBuff = nullptr;
		inputLen = 0;
		Bt8timeStamp = 0;
		_CompositionTime = 0;
		int nOffset = 0;
		USHORT nNaluSize;

		inputBuff = gVideoInfoStru->VidePacData;
		inputLen = gVideoInfoStru->WdBodyLen;
		Bt8timeStamp = gVideoInfoStru->Bt8timeStamp;
		_CompositionTime = gVideoInfoStru->timeStamp;
		// printf("************ Bt8timeStamp = %lu\n", Bt8timeStamp);

		while(true){
			nNaluSize = 0;

			nNaluSize = Cnvt::getNextNalu(inputBuff + nOffset, inputLen - nOffset);
			if(0 == nNaluSize)
				break;

			if(nNaluSize > 4){
				if(!sendVideoData(inputBuff + nOffset, nNaluSize)) 
				return false;
			}

			nOffset += nNaluSize;
			if (nOffset >= inputLen - 4)
				break;
		}
		return true;
	}

	void CConverter::MakeFlvHeader(unsigned char *pFlvHeader)
	{
		pFlvHeader[0] = 0X46; //'F';
		pFlvHeader[1] = 0X4C; //'L';
		pFlvHeader[2] = 0x56; //'V';
		pFlvHeader[3] = 0x01;
		pFlvHeader[4] = 0x01; //0x05

		unsigned int size = 9;
		u4 size_u4(size);
		memcpy(pFlvHeader + 5, size_u4._u, sizeof(unsigned int));
	}

	bool CConverter::sendFLVhead(int fd)
	{
		int ret = 0;
		ret = write(fd, _FlvHeader, 9);
		if(ret < 0) return false;

		// fwrite(_FlvHeader, 1, 9, file);
		return true;
	}

	int CConverter::WriteH264Header(unsigned int _TimeStamp, int fd, int _nPrevTagSize1)  //AVCC
	{
		memset(body, 0, MAX_BUFF_SIZE_);
		int i = 0;

		u4 prev_u4(_nPrevTagSize1);
		memcpy(body, prev_u4._u, 4);
		i += 4;

		body[i++] = 0x09;

		int nDataSize = 1 + 1 + 3 + 6 + 2 + (_nSPSSize - 4) + 1 + 2 + (_nPPSSize - 4);
		u3 datasize_u3(nDataSize);
		memcpy(&body[i], datasize_u3._u, 3);
		i += 3;

		u3 tt_u3(_TimeStamp); 
		memcpy(&body[i], tt_u3._u, 3);
		i += 3;
		unsigned char cTTex = _TimeStamp >> 24; 
		body[i++] = cTTex;
		// body[i++] = (unsigned char)(_TimeStamp >> 24);

		u3 sid_u3(_nStreamID);
		memcpy(&body[i], sid_u3._u, 3);
		i += 3;

		body[i++] = 0x17;
		body[i++] = 0x00;

		//CTS
		body[i++] = 0x00;
		body[i++] = 0x00;
		body[i++] = 0x00;

		body[i++] = 0x01;
		body[i++] = _pSPS[5];
		body[i++] = _pSPS[6];
		body[i++] = _pSPS[7];
		body[i++] = 0xff;
		body[i++] = 0xE1;
		
		if(_pSPS == nullptr || _nSPSSize <= 4) return 0;
		u2 spssize_u2(_nSPSSize - 4);
		memcpy(&body[i], spssize_u2._u, 2);
		i += 2;
		memcpy(&body[i], _pSPS + 4, _nSPSSize - 4); 
		i += (_nSPSSize - 4);

		body[i++] = 0x01;

		if(_pPPS == nullptr || _nPPSSize<= 4) return 0;
		u2 ppssize_u2(_nPPSSize - 4);
		memcpy(&body[i], ppssize_u2._u, 2);
		i += 2;
		memcpy(&body[i], _pPPS + 4, _nPPSSize - 4);
		i += (_nPPSSize - 4);

		int rePrevTagSize = 0;
		rePrevTagSize = 11 + nDataSize;

		int ret = 0;
		int count = 0;

		do{
			ret = write(fd, body + count, i - count);
			if(ret < 0) return 0;  //ret == -1
			count += ret;
		}while ((0 < count) && (count < i));
		// fwrite(body, 1, i, file);

		return rePrevTagSize;
	}

	int CConverter::WriteH264Frame(uint8_t* pNalu, USHORT nNaluSize, unsigned int nTimeStamp, int fd, int PrevTagSize, UINT CompositionTime)
	{
		int nNaluType = pNalu[4] & 0x1f;
		if (nNaluType == 7 || nNaluType == 8)
			return true;
		
		pNalu += 4;
		nNaluSize -= 4;
		memset(body, 0, MAX_BUFF_SIZE_);
		int i = 0;

		u4 prev_u4(PrevTagSize);
		memcpy(body, prev_u4._u, 4);
		i += 4;

		body[i++] = 0x09;

		int nDataSize;
		nDataSize = 1 + 1 + 3 + 4 + nNaluSize;
		u3 datasize_u3(nDataSize);
		memcpy(&body[i], datasize_u3._u, 3);
		i += 3;

		u3 tt_u3(nTimeStamp);
		memcpy(&body[i], tt_u3._u, 3);
		i += 3;
		body[i++] = (unsigned char)(nTimeStamp >> 24);

		u3 sid(_nStreamID);
		memcpy(&body[i], sid._u, 3);
		i += 3;

		if (nNaluType == 5)
			body[i++] = 0x17;
		else
			body[i++] = 0x27;

		body[i++] = 0x01;
		u3 com_time_u3(0); //CTS CompositionTime 200
		memcpy(&body[i], com_time_u3._u, 3);
		i += 3;

		u4 nalusize_u4(nNaluSize);
		memcpy(&body[i], nalusize_u4._u, 4);
		i += 4;

		// memcpy(&body[i], pNalu, nNaluSize);
		// i += nNaluSize;

		int _rePrevTagSize = 0;
		_rePrevTagSize = 11 + nDataSize;

		int ret = 0;
		int count = 0;
		do{
			ret = write(fd, body + count, i - count);
			if(ret < 0) return 0;
			count += ret;
		}while ((0 < count) && (count < i));
		// fwrite(body, 1, i, file);

		ret = 0;
		count = 0;
		do{
			ret = write(fd, pNalu + count, nNaluSize- count);
			if(ret < 0) return 0;
			count += ret;
		}while ((0 < count) && (count < nNaluSize));
		// fwrite(pNalu, 1, nNaluSize, file);

		return _rePrevTagSize;
	}



	int CConverter::ScriptTag()
	{
		memset(body, 0, MAX_BUFF_SIZE_);
		int i = 0;

		body[i++] = 0X46; //'F';
		body[i++] = 0X4C; //'L';
		body[i++] = 0x56; //'V';
		body[i++] = 0x01;
		body[i++] = 0x01; //0x05

		body[i++] = 0x00;
		body[i++] = 0x00;
		body[i++] = 0x00;
		body[i++] = 0x09;

		//PrevTag Size
		u4 prev_u4(0);
		memcpy(&body[i], prev_u4._u, 4);
		i += 4;

		body[i++] = 0x12;

		//body size 3B
		int nDataSize;
		nDataSize = 365;  //待改
		u3 datasize_u3(nDataSize);
		memcpy(&body[i], datasize_u3._u, 3);
		i += 3;

		//nTimeStamp 4B
		body[i++] = 0x00;
		body[i++] = 0x00;
		body[i++] = 0x00;
		body[i++] = 0x00;

		//StreamID 3B
		u3 sid(_nStreamID);
		memcpy(&body[i], sid._u, 3);
		i += 3;

		//Metadata  AMF_1 13B
		body[i++] = 0x02;
		body[i++] = 0x00;
		body[i++] = 0x0A;
		memcpy(&body[i], "onMetaData", 10); //0x6F 6E 4D 65 74 61 44 61 74 61
		i += 10;

		//Metadata  AMF_2 5B
		body[i++] = 0x08;
		body[i++] = 0x00;
		body[i++] = 0x00;
		body[i++] = 0x00;
		body[i++] = 0x07;   

		//duration 19B
		body[i++] = 0x00;
		body[i++] = 0x08;
		memcpy(&body[i], "duration", 8);
		i += 8;
		body[i++] = 0x00;
		i += 8; //duration值

		//width 16B
		body[i++] = 0x00;
		body[i++] = 0x05;
		memcpy(&body[i], "width", 5);
		i += 5;
		body[i++] = 0x00;
		i += 8; //width值

		//height 17B
		body[i++] = 0x00;
		body[i++] = 0x06;
		memcpy(&body[i], "height", 6);
		i += 6;
		body[i++] = 0x00;
		i += 8; //height值

		//videodatarate 24B
		body[i++] = 0x00;
		body[i++] = 0x0D;
		memcpy(&body[i], "videodatarate", 13);
		i += 13;
		body[i++] = 0x00;
		i += 8; //videodatarate 值

		//framerate 20B
		body[i++] = 0x00;
		body[i++] = 0x09;
		memcpy(&body[i], "framerate", 9);
		i += 9;
		body[i++] = 0x00;
		i += 8; //framerate 值

		//videocodecid 23B
		body[i++] = 0x00;
		body[i++] = 0x0C;
		memcpy(&body[i], "videocodecid", 12);
		i += 12;
		body[i++] = 0x00;
		i += 8; //videocodecid 值

		//filesize 22B
		body[i++] = 0x00;
		body[i++] = 0x08;
		memcpy(&body[i], "filesize", 8);
		i += 8;
		body[i++] = 0x00;
		i += 8; //filesize 值
		body[i++] = 0x00;
		body[i++] = 0x00;
		body[i++] = 0x09;


		int _rePrevTagSize = 0;
		_rePrevTagSize = 11 + nDataSize;
		return _rePrevTagSize;
	}





	void CConverter::WriteH264EndofSeq(int fd)
	{
		uint8_t body[SEND_SPS_PPS_BUFF_SIZE];
		memset(body, 0, SEND_SPS_PPS_BUFF_SIZE);
		int i = 0;

		u4 prev_u4(_nPrevTagSize);
		memcpy(body, prev_u4._u, 4);
		i += 4;

		body[i++] = 0x09;
		int nDataSize;
		nDataSize = 1 + 1 + 3;
		u3 datasize_u3(nDataSize);
		memcpy(&body[i], datasize_u3._u, 3);
		i += 3;
		u3 tt_u3(_nVideoTimeStamp);
		memcpy(&body[i], tt_u3._u, 3);
		i += 3;
		body[i++] = (unsigned char)(_nVideoTimeStamp >> 24);

		u3 sid(_nStreamID);
		memcpy(&body[i], sid._u, 3);
		i += 3;
		
		body[i++] = 0x27;
		body[i++] = 0x02;

		u3 com_time_u3(0);
		memcpy(&body[i], com_time_u3._u, 3);
		i += 3;

		// _fileOut.write((char *)body, i);

		int ret;
		ret = write(fd, body, i);
	}









	int CConverter::ConvertAAC(char *pAAC, int nAACFrameSize, unsigned int nTimeStamp)
	{
		if (pAAC == NULL || nAACFrameSize <= 7)
			return 0;

		if (_pAudioSpecificConfig == NULL)
		{
			_pAudioSpecificConfig = new unsigned char[2];
			_nAudioConfigSize = 2;

			unsigned char *p = (unsigned char *)pAAC;
			_aacProfile = (p[2] >> 6) + 1;
			_sampleRateIndex = (p[2] >> 2) & 0x0f;
			_channelConfig = ((p[2] & 0x01) << 2) + (p[3]>>6);

			_pAudioSpecificConfig[0] = (_aacProfile << 3) + (_sampleRateIndex>>1);
			_pAudioSpecificConfig[1] = ((_sampleRateIndex&0x01)<<7) + (_channelConfig<<3);
		}
		if (_pAudioSpecificConfig != NULL & _bWriteAACSeqHeader == 0)
		{
			WriteAACHeader(nTimeStamp);
			_bWriteAACSeqHeader = 1;
		}
		if (_bWriteAACSeqHeader == 0)
			return 1;

		WriteAACFrame(pAAC, nAACFrameSize, nTimeStamp);

		return 1;
	}

	void CConverter::WriteAACHeader(unsigned int nTimeStamp)
	{
		u4 prev_u4(_nPrevTagSize);
		_fileOut.write((char *)prev_u4._u, 4);

		char cTagType = 0x08;
		_fileOut.write(&cTagType, 1);
		int nDataSize = 1 + 1 + 2;

		u3 datasize_u3(nDataSize);
		_fileOut.write((char *)datasize_u3._u, 3);

		u3 tt_u3(nTimeStamp);
		_fileOut.write((char *)tt_u3._u, 3);

		unsigned char cTTex = nTimeStamp >> 24;
		_fileOut.write((char *)&cTTex, 1);

		u3 sid_u3(_nStreamID);
		_fileOut.write((char *)sid_u3._u, 3);

		unsigned char cAudioParam = 0xAF;
		_fileOut.write((char *)&cAudioParam, 1);
		unsigned char cAACPacketType = 0; /* seq header */
		_fileOut.write((char *)&cAACPacketType, 1);

		_fileOut.write((char *)_pAudioSpecificConfig, 2);

		_nPrevTagSize = 11 + nDataSize;
	}

	void CConverter::WriteAACFrame(char *pFrame, int nFrameSize, unsigned int nTimeStamp)
	{
		u4 prev_u4(_nPrevTagSize);
		Write(prev_u4);

		Write(0x08);
		int nDataSize;
		nDataSize = 1 + 1 + (nFrameSize - 7);
		u3 datasize_u3(nDataSize);
		Write(datasize_u3);
		u3 tt_u3(nTimeStamp);
		Write(tt_u3);
		Write((unsigned char)(nTimeStamp >> 24));

		u3 sid(_nStreamID);
		Write(sid);

		unsigned char cAudioParam = 0xAF;
		_fileOut.write((char *)&cAudioParam, 1);
		unsigned char cAACPacketType = 1; /* AAC raw data */
		_fileOut.write((char *)&cAACPacketType, 1);

		_fileOut.write((char *)pFrame + 7, nFrameSize - 7);

		_nPrevTagSize = 11 + nDataSize;
	}
}
