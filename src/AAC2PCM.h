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

#define HTON16(x)  ((x>>8&0xff)|(x<<8&0xff00))
#define HTON24(x)  ((x>>16&0xff)|(x<<16&0xff0000)|(x&0xff00))
#define HTON32(x)  ((x>>24&0xff)|(x>>8&0xff00)|(x<<8&0xff0000)|(x<<24&0xff000000))
#define HTONTIME(x) ((x>>16&0xff)|(x<<16&0xff0000)|(x&0xff00)|(x&0xff000000))

#pragma pack(1)
typedef struct _FLVHead
{
    char type[3];
    char version;
    char stream_info;
    int offset;
}FLVHead;
 
typedef struct _FLVTag
{
    int tag_size;
    char type;
    char length[3];
    char timecamp[3];
    char timecampex;
    char StreamsID[3];
}FLVTag;
#pragma pack()

void Header_info(FLVHead  flvHead);

#pragma pack (1)
typedef struct Audio_HEADER
{
    // unsigned            DWFramHeadMark;          //帧头标识
	// uint8_t sync[4] = {0x30, 0x31, 0x63, 0x64}; 
	unsigned char       DWFramHeadMark[4] = {0x30, 0x31, 0x63, 0x64};
/*    unsigned char       V2:2;                    //固定为2
    unsigned char       P1:1;                    //固定为0
    unsigned char       X1:1;                    //RTP头是否需要扩展位，固定为0
    unsigned char       CC4:4;                   //固定为1
    unsigned char       M1:1;                    //标志位，确定是否是完整数据帧的边界
    unsigned char       PT7:7;                   //负载类型
*/  
	unsigned char		info1;
	unsigned char		info2;
	unsigned short      WdPackageSequence;        //RTP数据包序号每发送一个RTP数据包序列号加1
    unsigned char       BCDSIMCardNumber[10];     //SIM卡号BCDSIMCardNumber[10];
    unsigned char       Bt1LogicChannelNumber;     //逻辑通道号
//    unsigned char       DataType4:4;                //数据类型
//    unsigned char       subpackageHandleMark4:4;    //分包处理标记
	unsigned char		info3;
    unsigned long int   Bt8timeStamp;              //时间戳
    unsigned short      WdBodyLen;                 //数据体长度

}AUDIO_HEADER,*AUDIO_HEADER_P;
#pragma pack()

#pragma pack (1)
typedef struct Audio_HEADER_S
{
  	// unsigned            DWFramHeadMark;          //帧头标识
	unsigned char       DWFramHeadMark[4] = {0x30, 0x31, 0x63, 0x64};
/*  unsigned char       V2:2;                    //固定为2
    unsigned char       P1:1;                    //固定为0
    unsigned char       X1:1;                    //RTP头是否需要扩展位，固定为0
    unsigned char       CC4:4;                   //固定为1
    unsigned char       M1:1;                    //标志位，确定是否是完整数据帧的边界
    unsigned char       PT7:7;                   //负载类型
*/
	unsigned char		info1;
	unsigned char		info2;
	unsigned short      WdPackageSequence;        //RTP数据包序号每发送一个RTP数据包序列号加1
  	unsigned char       BCDSIMCardNumber[6];     //SIM卡号BCDSIMCardNumber[6];
    unsigned char       Bt1LogicChannelNumber;     //逻辑通道号
//    unsigned char       DataType4:4;                //数据类型
//    unsigned char       subpackageHandleMark4:4;    //分包处理标记
	unsigned char		info3;
    unsigned long int   Bt8timeStamp;              //时间戳
    unsigned short      WdBodyLen;                 //数据体长度

}AUDIO_HEADER_S,*AUDIO_HEADER_PS;
#pragma pack()



#define FRAME_MAX_LEN 1024*5 
#define BUFFER_MAX_LEN 1024*1024
enum AccDataStatus{
	AccDataStatus_NotKnown			=		0x00,			//未知
	AccDataStatus_InValid			=		0x01,			//非法
	AccDataStatus_Valid				=		0x02,			//合法
};

typedef unsigned char  BYTE;
class AAC2PCM
{
public:
	AAC2PCM(int gsockfd, unsigned char gchan, BYTE* SIM, int audi_type);
	virtual ~AAC2PCM();

	int get_one_ADTS_frame(unsigned char* buffer, size_t buf_size, unsigned char* data ,size_t* data_size);
	int init(unsigned char defObjectType=2, unsigned long defSampleRate = 8000);
	int Decoder(unsigned char *pszAAC, unsigned int nLen, char *pszOut, int *pnOutLen);
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
	BYTE* AACData;

private:
	int   iRet;
	BYTE* ucOutBuff;
	BYTE* G711Buff;
	BYTE* writebuff;
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

/*	//test
public:
	char* file_buf;
	int file_size;
	int fd_input;
	unsigned char frame[FRAME_MAX_LEN];  
    unsigned char frame_mono[FRAME_MAX_LEN];
	int fout=0;
	int  data_size = 0;
	size_t size = 0;
	unsigned char* input_data;
	unsigned char* pcm_data = NULL;

	int open_file(char* file_input);
	int close_file(void);
	int file_mem_alloc(void);
	int read_file_to_mem(void);
	int file_mem_free(void);
	void test_init();
*/
};




#endif
