#include "shar_http.h"

sharHttpSer::sharHttpSer(const CONFIG ServerConfig)
{
    std::string httpserver = ServerConfig.httpserver;
    requestUrl = httpserver+"/mp02/state/getTalkingInfo";
    updateUrl = httpserver + "/mp02/state/updateTalkingState";

    std::string aiserver = ServerConfig.aiserver;
    voiceUrl = aiserver+"/service.ai/textToVoice/talkFile/";
}

sharHttpSer::~sharHttpSer()
{
}

// 回调函数，用于处理接收到的数据  static
size_t sharHttpSer::WriteCallback(void *contents, size_t size, size_t nmemb, std::string *userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}


bool sharHttpSer::POST_request(std::string device, std::string& ID)
{
    char postData[128] = {'\0'};
    sprintf(postData, "{\"deviceCode\":\"%s\"}", device.data());
    printf("deviceCode : %s\n", postData);
    readBuffer.clear();
 
    ID.clear();
    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL * curl = curl_easy_init();
    if (curl) {
        // 设置URL
        curl_easy_setopt(curl, CURLOPT_URL, requestUrl.c_str());
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
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        } else {
            std::cout << "Response: " << readBuffer << std::endl;
            getGoupId(ID);
        }
        // 清理CURL列表
        curl_slist_free_all(plist);
        // 清理CURL对象
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();

#if DEBUG
    ID = "debug_test_group";
#endif

    if(ID.empty()) ID = device;//AI对讲,以SIMh卡号虚拟出一个groupID
    return !ID.empty();
}

void sharHttpSer::getGoupId(std::string& ID)
{
	Json::Reader reader;
	Json::Value value;

	if (reader.parse(readBuffer, value)){
		int code = value["code"].asInt();
        ID = value["data"]["id"].asString();
	}
}

bool sharHttpSer::POST_update(std::string device, int state)
{
    bool result = false;
	char postData[128] = {'\0'};
	sprintf(postData, "{\"deviceCode\":\"%s\",\"state\":%d}", device.data(), state);

    CURL* curl = curl_easy_init();
	if (curl)
	{	
		curl_easy_setopt(curl, CURLOPT_URL, updateUrl.c_str());
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
			std::cout << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
		}
		else
		{
			std::cout << "strResponse is: " << strResponse << std::endl;
            result = getResult(strResponse);
		}

        curl_slist_free_all(plist);
		curl_easy_cleanup(curl);
	}
    return result;
}

std::vector<uint8_t> sharHttpSer::POST_pcm(const std::string& device, const std::vector<uint8_t>& pcmData) {
    std::vector<uint8_t> responseData;
    CURL* curl = curl_easy_init();
 
    if (!curl) {
        return responseData; // 返回空向量表示錯誤
    }
 
    std::string url = voiceUrl + device;
 
    // 設置cURL選項
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
 
    // 創建一個臨時的cURL文件句柄，用於上傳數據
    curl_mimepart* mimePart;
    curl_mime* mime = curl_mime_init(curl);
    mimePart = curl_mime_addpart(mime);
 
    // 設置文件名和數據
    curl_mime_name(mimePart, "file"); // 表單字段名稱
    curl_mime_filename(mimePart, "file"); // 虛擬文件名
    curl_mime_data(mimePart, reinterpret_cast<const char*>(pcmData.data()), pcmData.size());
 
    // 將MIME數據設置到cURL
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
 
    // 設置響應數據的回調函數
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, BufferWriterFunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);
 
    // 執行請求
    CURLcode res = curl_easy_perform(curl);
 
    // 清理cURL句柄和MIME數據
    curl_mime_free(mime);
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
