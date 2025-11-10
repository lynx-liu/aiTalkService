#ifndef _SHAR_HTTP_H
#define _SHAR_HTTP_H
#include "../json/json.h"
#include "config.h"
#include <curl/curl.h>
#include <map>
#include <list>

// 响应头信息
struct ResponseHeader {
    std::string ad_hear_record_id;
    std::string x_file_type;
};


class sharHttpSer
{
public:
    sharHttpSer(const CONFIG ServerConfig);
    ~sharHttpSer();

    bool getTalkingInfo(std::string device, std::string& ID, int& type);
    bool updateTalkingState(std::string device, int state);
    bool updateVoiceState(std::string id, std::string status, int playingTime);
    std::vector<uint8_t> POST_pcm(const std::string& device, const std::vector<uint8_t>& pcmData, ResponseHeader& responseHeader);
private:
    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *userp);
    static size_t BufferWriterFunc(void* contents, size_t size, size_t nmemb, std::vector<uint8_t>* userdata);
    static size_t HeaderWriterFunc(void* contents, size_t size, size_t nmemb, ResponseHeader* userdata);
    void getGoupId(std::string& ID, int& type);
    int getResult(std::string strResponse);
    std::string readBuffer;
    std::string requestUrl;
    std::string updateUrl;
    std::string voiceUrl;
    std::string voiceStatusUrl;
};




#endif