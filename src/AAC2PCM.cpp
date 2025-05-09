#include "AAC2PCM.h"

static unsigned char frame[FRAME_MAX_LEN] = {0};
unsigned int framesize = FRAME_MAX_LEN;

AAC2PCM::AAC2PCM(int gsockfd, unsigned char gchan, BYTE* SIM, int audi_type):
sockfd(gsockfd),
chan(gchan),
Audi_type(audi_type)
{
	memcpy(sim, SIM, 6);
	channels   = 1;
	decoder    = 0;
	PCMLen     = 0;
	m_bInit    = AAC_DEC_INIT_OFF;
	PCMBuff    = new unsigned char[PCM_BUFF_MAX]();
	ucOutBuff  = new BYTE[CU_OUT_BUFF_LEN]();
	G711Buff   = new BYTE[G711_BUFF_LEN]();
	writebuff  = new BYTE[WRITE_BUFF_SIZE]();
	audiheader = new AUDIO_HEADER();
	AACDasize = 1024*1024;
	AACData = new BYTE[AACDasize]();
	m_nFirstPackageAccDataStatus = AccDataStatus_NotKnown;
    init(LC,8000);
}

AAC2PCM::~AAC2PCM()
{
    NeAACDecClose(decoder);
	if(PCMBuff != nullptr){
		delete [] PCMBuff;
		PCMBuff = nullptr;
	}
	if(ucOutBuff != nullptr){
		delete [] ucOutBuff;
		ucOutBuff = nullptr;
	}
	if(G711Buff != nullptr){
		delete [] G711Buff;
		G711Buff = nullptr;
	}
	if(writebuff != nullptr){
		delete [] writebuff;
		writebuff = nullptr;
	}
	if(AACData != nullptr){
		delete [] AACData;
		AACData = nullptr;
	}
	if(audiheader != nullptr){
		delete audiheader;
		audiheader = nullptr;
	}
}

int AAC2PCM::init(unsigned char defObjectType,unsigned long defSampleRate)
{
	m_nFirstPackageAccDataStatus = AccDataStatus_NotKnown;
	decoder = NeAACDecOpen();
	NeAACDecConfigurationPtr conf = NeAACDecGetCurrentConfiguration(decoder);
	conf->defObjectType = LC;
	conf->defSampleRate = 8000; 
	conf->outputFormat = FAAD_FMT_16BIT; 
	conf->dontUpSampleImplicitSBR = 1;
	unsigned char nRet = NeAACDecSetConfiguration(decoder, conf);
	m_bNeAACDecInit = false;

	return 0;
}

int AAC2PCM::Decoder(unsigned char* bufferAAC, size_t buf_sizeAAC)
{
	if(AAC_DEC_INIT_OFF == m_bInit){
		if (NeAACDecInit(decoder, bufferAAC, buf_sizeAAC, &gsamplerate, &channels) < 0){
			printf("NeAACDecInit ERROR\n"); 
            return -1;  
        };
		m_bInit = AAC_DEC_INIT_ON;
	}
	pcm_data = nullptr;
	int   nDecodeLen = 0;
	pcm_data = (unsigned char*)NeAACDecDecode(decoder, &frame_info, bufferAAC, buf_sizeAAC);
	if (frame_info.error != 0 || frame_info.samples == 0){   
        printf("NeAACDecDecode ERROR\n");
        return buf_sizeAAC;  
    }
	nDecodeLen = frame_info.channels * frame_info.samples;
	memset(PCMBuff, 0, PCM_BUFF_MAX);
	memcpy(PCMBuff, pcm_data, nDecodeLen);
	EncodeG711(nDecodeLen);
	return nDecodeLen;
}

bool AAC2PCM::EncodeG711(int DecodeLen)
{
	iRet = 0;
	iRet = g711a_encode(ucOutBuff, (short*)PCMBuff, DecodeLen / 2);
	memcpy(G711Buff+G711BUFFLen, ucOutBuff, iRet);
	G711BUFFLen += iRet;
	send_to_device();
	return true;
}

bool AAC2PCM::send_to_device()
{
	unsigned HeaLeng = sizeof(AUDIO_HEADER);
	memset(audiheader, 0, HeaLeng);
	// audiheader->DWFramHeadMark = 0x64633130;  //临时屏蔽测试
	audiheader->info1          = 0x81;
	if(Audi_type == LOAD_TYPE_G711A_TYPE){
		audiheader->info2 = 0x86;              //G.711A
	}else if(Audi_type == LOAD_TYPE_G726_TYPE){
		audiheader->info2 = 0x88;              //G.726
	}
	memcpy(audiheader->BCDSIMCardNumber, sim, 6);
	audiheader->Bt1LogicChannelNumber = chan;
	audiheader->info3          = 0x30;
	audiheader->WdBodyLen = SEND_DATA_PACK_SIZE;
	audiheader->WdBodyLen = htons(audiheader->WdBodyLen);
	while(G711BUFFLen >= SEND_DATA_PACK_SIZE){
		num++;
		audiheader->WdPackageSequence = htons(num);
		gettimeofday(&tv, NULL);
		timestamp = (tv.tv_sec * 1000 + tv.tv_usec / 1000);
		audiheader->Bt8timeStamp = htonl(timestamp);

		memset(writebuff, 0, HeaLeng);
		memcpy(writebuff, audiheader, HeaLeng);
		memcpy(writebuff + HeaLeng, G711Buff, SEND_DATA_PACK_SIZE);
		wriRet = write(sockfd, writebuff, HeaLeng+ SEND_DATA_PACK_SIZE);
		usleep(40000);
		G711BUFFLen -= SEND_DATA_PACK_SIZE;
		memcpy(G711Buff, G711Buff+SEND_DATA_PACK_SIZE, G711BUFFLen);
	}
}

int AAC2PCM::get_one_ADTS_frame(unsigned char* buffer, size_t buf_size, unsigned char* data ,size_t* data_size)
{
    size_t size = 0;
    if(!buffer || !data || !data_size )
        return -1;

    while(1)
    {
        if(buf_size  < 7 )
            return -1;
		if ((buffer[0] == 0xff) && ((buffer[1] & 0xf0) == 0xf0)){
			size |= (((buffer[3] & 0x03)) << 11);
			size |= (buffer[4] << 3);
			size |= ((buffer[5] & 0xe0) >> 5);

			printf("len1=%x\n", (buffer[3] & 0x03));
			printf("len2=%x\n", buffer[4]);
			printf("len3=%x\n", (buffer[5] & 0xe0) >> 5);
			printf("size=%d\r\n", (int)size);
			break;
		}
        --buf_size;
        ++buffer;
    }

    if(buf_size < size){
        return -1;
    }
    memcpy(data, buffer, size);
    *data_size = size;
    return 0;
}


//检测数据是否合法
int AAC2PCM::detectFirstPackageData(unsigned char* bufferAAC, size_t buf_sizeAAC)
{
	size_t size = 0;
	if(get_one_ADTS_frame(bufferAAC, buf_sizeAAC, frame, &size) < 0){
		m_nFirstPackageAccDataStatus = AccDataStatus_InValid;
		return -1;
	}
	m_nFirstPackageAccDataStatus  = AccDataStatus_Valid;
	return 0;
}
int AAC2PCM::getFirstPackageAccDataStatus()
{
	return m_nFirstPackageAccDataStatus;
}
//重置第一数据包状态
void AAC2PCM::clearFirstPackageAccDataStatus(int nAccDataStatus)
{
	m_nFirstPackageAccDataStatus = nAccDataStatus;
}

int AAC2PCM::convert(unsigned char* bufferAAC, size_t buf_sizeAAC,unsigned char* bufferPCM, size_t & buf_sizePCM)
{
	if (m_nFirstPackageAccDataStatus != AccDataStatus_Valid) 
        return -1;
	size_t size = 0;
	pcm_data = nullptr;
	while(get_one_ADTS_frame(bufferAAC, buf_sizeAAC, frame, &size) == 0)
	{
		pcm_data = (unsigned char*)NeAACDecDecode(decoder, &frame_info, frame, size);
		if(frame_info.error > 0){
			printf("%s\n",NeAACDecGetErrorMessage(frame_info.error));            
			return -1;
		}else if(pcm_data && frame_info.samples > 0){
			printf("frame info: bytesconsumed %d, channels %d, header_type %d\
				   object_type %d, samples %d, samplerate %d\n", 
				   frame_info.bytesconsumed, 
				   frame_info.channels, frame_info.header_type, 
				   frame_info.object_type, frame_info.samples, 
				   frame_info.samplerate);

			buf_sizePCM = frame_info.samples * frame_info.channels;
			memcpy(bufferPCM,pcm_data,buf_sizePCM);
		}        
		bufferAAC -= size;
		buf_sizeAAC += size;
	}

	return 0;
}

int AAC2PCM::convert2(unsigned char* bufferAAC, size_t buf_sizeAAC, unsigned char* bufferPCM, size_t & buf_sizePCM)
{
	pcm_data = nullptr;
	if (!m_bNeAACDecInit){
		NeAACDecInit(decoder, bufferAAC, buf_sizeAAC, &gsamplerate, &channels);
		printf("samplerate %d, channels %d\n", gsamplerate, channels);
		m_bNeAACDecInit = true;
	}
	pcm_data = (unsigned char*)NeAACDecDecode(decoder, &frame_info, bufferAAC, buf_sizeAAC);
	if (frame_info.error > 0){
		printf("%s\n", NeAACDecGetErrorMessage(frame_info.error));
		return -1;
	}else if (pcm_data && frame_info.samples > 0){
		printf("frame info: bytesconsumed %d, channels %d, header_type %d\
			   				   object_type %d, samples %d, samplerate %d\n",
							   frame_info.bytesconsumed,
							   frame_info.channels, frame_info.header_type,
							   frame_info.object_type, frame_info.samples,
							   frame_info.samplerate);

		buf_sizePCM = frame_info.samples * frame_info.channels;
		memcpy(bufferPCM,pcm_data,buf_sizePCM);
		return 0;
	}

	return -1;
}






