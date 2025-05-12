#include "shar_http.h"

sharHttpSer::sharHttpSer(std::string baseUrl)
{
    requestUrl = baseUrl+"/mp02/state/getTalkingInfo";
    updateUrl = baseUrl + "/mp02/state/updateTalkingState";
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
    return !ID.empty();
}

void sharHttpSer::getGoupId(std::string& ID)
{
	Json::Reader reader;
	Json::Value value;

	if (reader.parse(readBuffer, value)){
		int code = value["code"].asInt();
		std::cout << "code = " << code << std::endl;
		std::cout << "msg = " << value["msg"].asString() << std::endl;
        std::cout << "name = " << value["data"]["name"].asString() << std::endl;
        std::cout << "id = " << value["data"]["id"].asString() << std::endl;
        std::cout << "type = " << value["data"]["type"].asInt() << std::endl;
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
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, BufferWriterFunc);

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

int sharHttpSer::BufferWriterFunc(char * data, size_t size, size_t nmemb, std::string * buffer)
{
	int result = 0;
	if (buffer != NULL)
	{
		buffer->append(data, size * nmemb);
		result = size * nmemb;
	}
	return result;
}

int sharHttpSer::getResult(std::string strResponse)
{
    int code = 0;
	Json::Reader reader;
	Json::Value value;

	if (reader.parse(strResponse, value)){
		code = value["code"].asInt();

		std::cout << "code = " << value["code"].asInt() << std::endl;
		std::cout << "msg = " << value["msg"].asString() << std::endl;
	}
    return code;
}
