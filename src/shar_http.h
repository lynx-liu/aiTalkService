#ifndef _SHAR_HTTP_H
#define _SHAR_HTTP_H
#include "../json/json.h"
#include "config.h"
#include <curl/curl.h>
#include <map>
#include <list>

class sharHttpSer
{
public:
    sharHttpSer(const CONFIG ServerConfig);
    ~sharHttpSer();

    bool POST_request(std::string device, std::string& ID);
    bool POST_update(std::string device, int state);
    std::vector<uint8_t> POST_pcm(const std::string& device, const std::vector<uint8_t>& pcmData);
private:
    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *userp);
    static size_t BufferWriterFunc(void* contents, size_t size, size_t nmemb, std::vector<uint8_t>* userdata);
    void getGoupId(std::string& ID);
    int getResult(std::string strResponse);
    std::string readBuffer;
    std::string requestUrl;
    std::string updateUrl;
    std::string voiceUrl;
};




#endif