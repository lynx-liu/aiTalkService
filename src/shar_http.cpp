#include "shar_http.h"
#include "debug.h"
#include <sstream>
#include <mutex>

namespace {

std::once_flag g_curl_init_once;

inline void ensure_curl_global_init() {
    std::call_once(g_curl_init_once, []() {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    });
}

} // namespace

sharHttpSer::sharHttpSer(const CONFIG ServerConfig)
{
    std::string httpserver = ServerConfig.httpserver;
    requestUrl = httpserver+"/mp02/state/getTalkingInfo";
    updateUrl = httpserver + "/mp02/state/updateTalkingState";

    std::string aiserver = ServerConfig.aiserver;
    voiceUrl = aiserver+"/service.ai/textToVoice/talkFile/";
    voiceStatusUrl = aiserver + "/service.ai/textToVoice/update";
}

sharHttpSer::~sharHttpSer()
{
}

// 回调函数，用于处理接收到的数据  static
size_t sharHttpSer::WriteCallback(void *contents, size_t size, size_t nmemb, std::string *userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

size_t sharHttpSer::HeaderWriterFunc(void* contents, size_t size, size_t nmemb, ResponseHeader* responseHeader) {
    size_t totalSize = size * nmemb;
    if (totalSize > 0) {
        std::string headerLine(reinterpret_cast<char*>(contents), totalSize);

        // 去掉行尾的 \r 和 \n
        while (!headerLine.empty() && 
              (headerLine.back() == '\r' || headerLine.back() == '\n')) {
            headerLine.pop_back();
        }

        if (headerLine.empty()) {
            // 空行 (headers 和 body 的分隔符)
            return totalSize;
        }

        auto pos = headerLine.find(':');
        if (pos != std::string::npos) {
            std::string headerName  = headerLine.substr(0, pos);
            std::string headerValue = headerLine.substr(pos + 1);

            // 去掉前后空格和控制字符
            headerName.erase(0, headerName.find_first_not_of(" \t\r\n"));
            headerName.erase(headerName.find_last_not_of(" \t\r\n") + 1);
            headerValue.erase(0, headerValue.find_first_not_of(" \t\r\n"));
            headerValue.erase(headerValue.find_last_not_of(" \t\r\n") + 1);

            //printf("\n%sHeader: %s: %s", getNowTime().data(), headerName.c_str(), headerValue.c_str());

            if (headerName == "ad_hear_record_id") {
                responseHeader->ad_hear_record_id = headerValue;
            } else if (headerName == "x-file-type") {
                responseHeader->x_file_type = headerValue;
            }
        }/* else {
            // 不是 key:value 结构 (状态行等)
            printf("\n%sHeader: %s", getNowTime().data(), headerLine.c_str());
        }*/
    }
    return totalSize;
}

bool sharHttpSer::getTalkingInfo(std::string device, std::string& ID, int& type)
{
    ensure_curl_global_init();

    char postData[128] = {'\0'};
    snprintf(postData, sizeof(postData), "{\"deviceCode\":\"%s\"}", device.c_str());
    printf("\n%sdeviceCode : %s", getNowTime().data(), postData);
    readBuffer.clear();
 
    ID.clear();
    CURL * curl = curl_easy_init();
    if (curl) {
        // 设置URL
        curl_easy_setopt(curl, CURLOPT_URL, requestUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
        // 设置POST请求
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        // 设置POST字段字符串
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData);
        // 设置Content-Type为application/json
        struct curl_slist *plist  = nullptr;
        plist = curl_slist_append(plist, "Content-Type: application/json; charset=utf-8");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, plist);
        // 设置回调函数来处理响应数据
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        // 执行请求并获取响应
        CURLcode res = curl_easy_perform(curl);
        // 检查错误
        if (res != CURLE_OK) {
            printf("\n%scurl_easy_perform() failed: %s", getNowTime().data(), curl_easy_strerror(res));
        } else {
            printf("\n%sResponse: %s", getNowTime().data(), readBuffer.data());
            getGoupId(ID, type);
        }
        // 清理CURL列表
        curl_slist_free_all(plist);
        // 清理CURL对象
        curl_easy_cleanup(curl);
    }

#if DEBUG
    ID = "debug_test_group";
#endif

    if(ID.empty()) ID = device;//AI对讲,以SIM卡号虚拟出一个groupID
    return !ID.empty();
}

void sharHttpSer::getGoupId(std::string& ID, int& type)
{
	Json::Reader reader;
	Json::Value value;

	if (reader.parse(readBuffer, value)){
		int code = value["code"].asInt();
        ID = value["data"]["id"].asString();
        type = value["data"]["type"].asInt();
	}
}

//1.链接成功 2.离线  3. 正在进行 4.结束, 5. 成为主讲人
bool sharHttpSer::updateTalkingState(std::string device, int state)
{
    ensure_curl_global_init();
    bool result = false;
	char postData[128] = {'\0'};
    snprintf(postData, sizeof(postData), "{\"deviceCode\":\"%s\",\"state\":%d}", device.c_str(), state);

    CURL* curl = curl_easy_init();
	if (curl)
	{	
		curl_easy_setopt(curl, CURLOPT_URL, updateUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
        curl_easy_setopt(curl,CURLOPT_POST, 1);
        struct curl_slist *plist = curl_slist_append(NULL, "Content-Type:application/json;charset=UTF-8");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, plist);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);

        std::string strResponse;
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &strResponse);
		
        CURLcode res = curl_easy_perform(curl);
		if (res != CURLE_OK)
		{
			printf("\n%s %scurl_easy_perform() failed: %s", device.c_str(), getNowTime().data(), curl_easy_strerror(res));
		}
		else
		{
			printf("\n%s %sstrResponse is: %s", device.c_str(), getNowTime().data(), strResponse.c_str());
            result = getResult(strResponse);
		}

        curl_slist_free_all(plist);
		curl_easy_cleanup(curl);
	}
    return result;
}

// 更新广告播放状态, status:[02:完成，03:中断]
bool sharHttpSer::updateVoiceState(std::string id, std::string status, int playingTime)
{
    ensure_curl_global_init();
    bool result = false;
	char postData[128] = {'\0'};
    snprintf(postData, sizeof(postData), "{\"id\":\"%s\",\"status\":%s,\"playingTime\":%d}", id.c_str(), status.c_str(), playingTime);

    CURL* curl = curl_easy_init();
	if (curl)
	{	
		curl_easy_setopt(curl, CURLOPT_URL, voiceStatusUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
        curl_easy_setopt(curl,CURLOPT_POST, 1);
        struct curl_slist *plist = curl_slist_append(NULL, "Content-Type:application/json;charset=UTF-8");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, plist);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);

        std::string strResponse;
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &strResponse);
		
        CURLcode res = curl_easy_perform(curl);
		if (res != CURLE_OK)
		{
			printf("\n%scurl_easy_perform() failed: %s", getNowTime().data(), curl_easy_strerror(res));
		}
		else
		{
			printf("\n%sstrResponse is: %s", getNowTime().data(), strResponse.data());
            result = getResult(strResponse);
		}

        curl_slist_free_all(plist);
		curl_easy_cleanup(curl);
	}
    return result;
}

std::vector<uint8_t> sharHttpSer::POST_pcm(const std::string& device, const std::vector<uint8_t>& pcmData, ResponseHeader& responseHeader) {
    ensure_curl_global_init();
    std::vector<uint8_t> responseData;
    CURL* curl = curl_easy_init();
    if (!curl) return responseData;

    std::string url = voiceUrl + device;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 25L);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);

    const char* boundary = "----Boundary123456789";  // 固定边界字符串

    // 构造 multipart/form-data 请求体
    std::string body;
    body += "--"; body += boundary; body += "\r\n";
    body += "Content-Disposition: form-data; name=\"file\"; filename=\"file\"\r\n";
    body += "Content-Type: application/octet-stream\r\n\r\n";
    body.append(reinterpret_cast<const char*>(pcmData.data()), pcmData.size());
    body += "\r\n--"; body += boundary; body += "--\r\n";

    // 设置请求体
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)body.size());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.data());

    // 设置头部
    struct curl_slist* headers = nullptr;
    std::string contentType = "Content-Type: multipart/form-data; boundary=" + std::string(boundary);
    headers = curl_slist_append(headers, contentType.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // 设定写回调和写入对象
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, BufferWriterFunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

    // 设定头回调和写入对象
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderWriterFunc);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &responseHeader);
        
    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        responseData.clear();
    }

    return responseData;
}

size_t sharHttpSer::BufferWriterFunc(void* contents, size_t size, size_t nmemb, std::vector<uint8_t>* userdata) {
    size_t totalSize = size * nmemb;
    if (totalSize > 0) {
        const uint8_t* data = reinterpret_cast<uint8_t*>(contents);
        userdata->insert(userdata->end(), data, data + totalSize);
    }
    return totalSize;
}

int sharHttpSer::getResult(std::string strResponse)
{
    int code = 0;
	Json::Reader reader;
	Json::Value value;

	if (reader.parse(strResponse, value))
		code = value["code"].asInt();
    return code;
}
