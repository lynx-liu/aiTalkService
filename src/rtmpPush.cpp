#include "rtmpPush.h"
rtmpPush::rtmpPush()
{   
}

rtmpPush::~rtmpPush()
{
	if(outputContext != nullptr){
		av_write_trailer(outputContext);
	}
	CloseInput();
	CloseOutput();
}

bool rtmpPush::init(char* url)
{
	rtmpUrl = url;

	ofmt = nullptr;
	ofmt_ctx = nullptr;

	stream_index = 0;
    waitI = 0;
    rtmpisinit = 0;
    ptsInc = 0;

	spsLen = 0;
	ppsLen = 0;
	status = FIRST_SEND_STATUS_OF;
	startTimeStamp = 0;

	videoindex  = -1;
	frame_index = 0;
	start_time  = 0;

	label = 0;
	outputContext = nullptr;
	inputContext  = nullptr;
	spsStatus = FIRST_NALU_TYPE_NON_SPS;

	av_register_all();
	return true;
}

bool rtmpPush::executeProcess2(SEND_VIDEO_INFO_STRU* gVideoInfoStru)
{
	inputBuff = nullptr;
	inputLen = 0;
	Bt8timeStamp = 0;
	_CompositionTime = 0;

	inputBuff = gVideoInfoStru->VidePacData;
	inputLen = gVideoInfoStru->WdBodyLen;
	Bt8timeStamp = gVideoInfoStru->Bt8timeStamp;
	_CompositionTime = gVideoInfoStru->timeStamp;

	if(inputLen > 4){
		if(sendVideoData2(inputBuff, inputLen)) 
			return true;
	}
	return false;
}

bool rtmpPush::sendVideoData2(uint8_t* pNalu, Cnvt::USHORT nNaluSize)
{
	int ret  = 0;
	naluType = 0;
	naluType = pNalu[4]&0x1f;

	if(spsStatus == FIRST_NALU_TYPE_NON_SPS){
		if(openInput()){
			if(openOutput()){
				spsStatus = FIRST_NALU_TYPE_YES_SPS;
				label = 1;
			}
		}
		if(label == 0){
			CloseInput();
			CloseOutput();
			return true;
		}
		startTimeStamp = Bt8timeStamp;
	}

	if(!readAndWrite()){
		printf("WritePacket failed!\n");
		return false;
	}

	return true;
}

int rtmpPush::read_Callback(void *opaque, uint8_t *buf, int buf_size)
{
	rtmpPush* _this = (rtmpPush*)opaque;
	memcpy(buf, _this->inputBuff, _this->inputLen);
	return _this->inputLen;
}

 bool rtmpPush::openInput()
 {
	lastReadPacktTime = av_gettime();
	int size = 60 * 1024;
	uint8_t * iobuffer = (uint8_t *)av_malloc(size);
	AVIOContext *avio = avio_alloc_context(iobuffer, size, 0, this, read_Callback, NULL, NULL);
	inputContext = avformat_alloc_context();
	inputContext->pb = avio;
	inputContext->start_time_realtime = av_gettime();

	int ret = avformat_open_input(&inputContext, nullptr, nullptr, nullptr);
	if(ret < 0){
		printf("Input file open input failed\n");
		return  false;
	}

	ret = avformat_find_stream_info(inputContext,nullptr);
	if(ret < 0){
		printf("Find input file stream inform failed\n");
		return false;
	}

	return true;
 }

bool rtmpPush::openOutput()
{
	int ret  = avformat_alloc_output_context2(&outputContext, nullptr, "flv", rtmpUrl.c_str());
	if(ret < 0){
		printf("open output context failed\n");
		goto Error;
	}

	ret = avio_open2(&outputContext->pb, rtmpUrl.c_str(), AVIO_FLAG_WRITE,nullptr, nullptr);	
	if(ret < 0){
		printf("open avio failed\n");
		goto Error;
	}

	for(int i = 0; i < inputContext->nb_streams; i++){
		AVStream * stream = avformat_new_stream(outputContext, inputContext->streams[i]->codec->codec);				
		ret = avcodec_copy_context(stream->codec, inputContext->streams[i]->codec);	
		stream->codec->codec_tag = 0;
		if(ret < 0){
			printf("copy coddec context failed\n");
			goto Error;
		}
	}

	ret = avformat_write_header(outputContext, nullptr);
	if(ret < 0){
		printf("format write header failed\n");
		goto Error;
	}

	printf(" Open output file success  %s\n", rtmpUrl.c_str());
	return true ;
Error:
	if(outputContext)
	{
		for(int i = 0; i < outputContext->nb_streams; i++)
		{
			avcodec_close(outputContext->streams[i]->codec);
		}
		avformat_close_input(&outputContext);
	}
	return false ;
}


void rtmpPush::CloseInput()
{
	if(inputContext != nullptr)
	{
		avformat_close_input(&inputContext);
		inputContext = nullptr;
	}
}

void rtmpPush::CloseOutput()
{
	if(outputContext != nullptr)
	{
		for(int i = 0 ; i < outputContext->nb_streams; i++)
		{
			AVCodecContext *codecContext = outputContext->streams[i]->codec;
			avcodec_close(codecContext);
		}
		// avformat_close_input(&outputContext);
		avformat_free_context(outputContext);
	}
	outputContext = nullptr;
}


int rtmpPush::readAndWrite()
{
	int ret;
	ret = av_read_frame(inputContext, &pkt);
	if(ret < 0) return 0;
	pkt.pts = Bt8timeStamp - startTimeStamp;

	ret = av_interleaved_write_frame(outputContext, &pkt);
	if (ret < 0) {
		printf("Error muxing packet\n");
		return 0;
	}
		
	// av_free_packet(&pkt);
	av_packet_unref(&pkt);
	return ret;
}
