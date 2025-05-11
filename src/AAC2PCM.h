#ifndef _AAC2PCM_H
#define _AAC2PCM_H
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include<unistd.h>
#include <cstring>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include "../include/faad.h"
#include "StreDataType.h"

#define AAC_DEC_INIT_OFF 0
#define AAC_DEC_INIT_ON  1
#define PCM_BUFF_MAX     4*1024
#define CU_OUT_BUFF_LEN  4*1024
#define G711_BUFF_LEN    5*1024
#define WRITE_BUFF_SIZE  1024
#define SEND_DATA_PACK_SIZE 160

#define LOAD_TYPE_G711A_TYPE  0x06   //G.711A
#define LOAD_TYPE_G726_TYPE   0x08   //G.726
#define LOAD_TYPE_ADPCM_TYPE 0x1A	//ADPCM

#define HTON16(x)  ((x>>8&0xff)|(x<<8&0xff00))
#define HTON24(x)  ((x>>16&0xff)|(x<<16&0xff0000)|(x&0xff00))
#define HTON32(x)  ((x>>24&0xff)|(x>>8&0xff00)|(x<<8&0xff0000)|(x<<24&0xff000000))
#define HTONTIME(x) ((x>>16&0xff)|(x<<16&0xff0000)|(x&0xff00)|(x&0xff000000))

#pragma pack(1)
typedef struct Audio_HEADER
{
	unsigned char       DWFramHeadMark[4] = {0x30, 0x31, 0x63, 0x64};
	unsigned char		info1;
	unsigned char		info2;
	unsigned short      WdPackageSequence;        //RTP数据包序号每发送一个RTP数据包序列号加1
    unsigned char       BCDSIMCardNumber[10];     //SIM卡号BCDSIMCardNumber[10];
    unsigned char       Bt1LogicChannelNumber;     //逻辑通道号
	unsigned char		info3;
    unsigned long int   Bt8timeStamp;              //时间戳
    unsigned short      WdBodyLen;                 //数据体长度

}AUDIO_HEADER,*AUDIO_HEADER_P;
#pragma pack()

#define FRAME_MAX_LEN 1024*5 

enum AccDataStatus{
	AccDataStatus_NotKnown			=		0x00,			//未知
	AccDataStatus_InValid			=		0x01,			//非法
	AccDataStatus_Valid				=		0x02,			//合法
};

class AAC2PCM
{
public:
	AAC2PCM(int gsockfd, unsigned char gchan, uint8_t* SIM, int audi_type);
	virtual ~AAC2PCM();

	int get_one_ADTS_frame(unsigned char* buffer, size_t buf_size, unsigned char* data ,size_t* data_size);
	int init(unsigned char defObjectType=2, unsigned long defSampleRate = 8000);
	int detectFirstPackageData(unsigned char* bufferAAC, size_t buf_sizeAAC);                                     //检测数据是否合法
	int getFirstPackageAccDataStatus();						                                                      //获取第一数据包状态
	void clearFirstPackageAccDataStatus(int nAccDataStatus);                                                      //重置第一数据包状态
	int convert(unsigned char* bufferAAC, size_t buf_sizeAAC,unsigned char* bufferPCM, size_t & buf_sizePCM);     //转换
	int convert2(unsigned char* bufferAAC, size_t buf_sizeAAC, unsigned char* bufferPCM, size_t & buf_sizePCM);
	int Decoder(unsigned char* bufferAAC, size_t buf_sizeAAC);

protected:
	unsigned long		gsamplerate;
	unsigned char		channels;
	NeAACDecHandle 		decoder;
	NeAACDecFrameInfo   frame_info;
	int m_nFirstPackageAccDataStatus;		                                                                      //第一数据包状态
	bool m_bNeAACDecInit;

	unsigned char* pcm_data;
	unsigned char* PCMBuff;
	int			   PCMLen;
	int			   m_bInit;
	unsigned char* PCMdata;
public:
	int   AACDasize;
	uint8_t* AACData;

private:
	uint8_t* ucOutBuff;
	uint8_t* G711Buff;
	uint8_t* writebuff;
	int	  OutBuffLen;
	int	  G711BUFFLen;
	bool EncodeG711(int DecodeLen);
	bool send_to_device();

private:
	AUDIO_HEADER*     audiheader;
	struct timeval    tv;
	ssize_t           wriRet;
	unsigned long int timestamp = 0;
	unsigned short    num = 0;
	std::string       audioName;
    unsigned char     sim[7] = {0};
    unsigned char     chan;
	int				  sockfd;
	int				  Audi_type;
};

#endif
