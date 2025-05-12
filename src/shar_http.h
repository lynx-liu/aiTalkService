#ifndef _SHAR_HTTP_H
#define _SHAR_HTTP_H
#include "../json/json.h"
#include <curl/curl.h>
#include <map>
#include <list>

class sharHttpSer
{
public:
    sharHttpSer(std::string baseUrl);
    ~sharHttpSer();

    bool POST_request(std::string device, std::string& ID);
    bool POST_update(std::string device, int state);
private:
    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *userp);
    static int BufferWriterFunc(char * data, size_t size, size_t nmemb, std::string * buffer);
    void getGoupId(std::string& ID);
    int getResult(std::string strResponse);
    std::string readBuffer;
    std::string requestUrl;
    std::string updateUrl;
};




#endif