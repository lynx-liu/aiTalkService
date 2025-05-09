#ifndef _SHAR_HTTP_H
#define _SHAR_HTTP_H
#include "../json/json.h"
#include <curl/curl.h>
#include <map>
#include <list>

class sharHttpSer
{

public:
    sharHttpSer(/* args */);
    ~sharHttpSer();

    bool POST_request(std::string device, std::string& ID);
    void http_Reinit();
    bool request_status();
    bool POST_update(std::string device, int state);
private:
    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *userp);
    static int BufferWriterFunc(char * data, size_t size, size_t nmemb, std::string * buffer);
    void Json_analyze();
    void Json_analyze2();
    std::string readBuffer;
    int _code;
    std::map<std::string, std::string> cluInfoMap;
    std::list<std::string> cluList;
    std::string devIDstr;
    std::string cluIDstr;
    std::string strResponse;
};




#endif