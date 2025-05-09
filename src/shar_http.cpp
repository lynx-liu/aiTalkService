#include "shar_http.h"

// #define URL "http://192.168.1.55:8084/mp02/state/getTalkingInfo"
// #define UPURL "http://192.168.1.55:8084/mp02/state/updateTalkingState"

// #define URL "http://192.168.0.101:8084/mp02/state/getTalkingInfo"
// #define UPURL "http://192.168.0.101:8084/mp02/state/updateTalkingState"

#define URL "https://wechat.che-mi.net/mp02/state/getTalkingInfo"
#define UPURL "https://wechat.che-mi.net/mp02/state/updateTalkingState"

sharHttpSer::sharHttpSer(/* args */)
{
}

sharHttpSer::~sharHttpSer()
{
}

void sharHttpSer::http_Reinit()
{
    _code = 0;
    devIDstr.clear();
    cluIDstr.clear();
    cluList.clear();
    // cluInfoMap.clear();
}

// 回调函数，用于处理接收到的数据  static
size_t sharHttpSer::WriteCallback(void *contents, size_t size, size_t nmemb, std::string *userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}


bool sharHttpSer::POST_request(std::string device, std::string& ID)
{
    _code = 0;
    CURL *curl;
    CURLcode res;
    char postData[128] = {'\0'};
    sprintf(postData, "{\"deviceCode\":\"%s\"}", device.data());
    printf("deviceCode : %s\n", postData);
    readBuffer.clear();
    
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        // 设置URL
        curl_easy_setopt(curl, CURLOPT_URL, URL);
        // 设置POST请求
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        // 设置POST字段字符串
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData);
        // 设置Content-Type为application/json
        // struct curl_slist *headers = NULL;
        struct curl_slist *plist  = nullptr;
        plist = curl_slist_append(plist, "Content-Type: application/json; charset=utf-8");
        // struct curl_slist *plist = curl_slist_append(plist, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, plist);
        // 设置回调函数来处理响应数据
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        // 执行请求并获取响应

        res = curl_easy_perform(curl);
        // 检查错误
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        } else {
            std::cout << "Response: " << readBuffer << std::endl;
            // readBuffer.clear();
            // readBuffer = \
            // "{\"msg\": \"查询成功\",\
            // \"code\": 804000,\
            // \"data\":\
            //  {\"memberList\": \
            //                 [{\"name\": \"司机名称1\",\"vehcileCode\": \"测DBD002\",\"deviceCode\": \"14654323697\"},{\"name\": \"司机名称2\",\"vehcileCode\": \"测DBD002\",\"deviceCode\": \"14654323697\"}],\
            //     \"name\": \"群组名称\",\"id\": \"000f6e46b74f4cb6bd7c827b9d4a53f7\",\"type\": 1}}";
            Json_analyze();
        }
        // 清理CURL列表
        curl_slist_free_all(plist);
        // 清理CURL对象
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();

    if(804000 == _code){
        ID = cluIDstr;
        return true;
    }
    return false;
}

void sharHttpSer::Json_analyze()
{
    devIDstr.clear();
    cluIDstr.clear();
    cluList.clear();
    // cluInfoMap.clear();

	Json::Reader reader;
	Json::Value value;

	if (reader.parse(readBuffer, value)){
		_code = value["code"].asInt();

		std::cout << "code = " << _code << std::endl;
		// std::cout << "authorizeCode = " << value["data"]["authorizeCode"] << std::endl;
		// std::string _deviceCode = value["data"]["deviceCode"].asString();
		// std::cout << "vehicleCode = " << _deviceCode << std::endl;
		std::cout << "msg = " << value["msg"].asString() << std::endl;

        // //获取数组个数
        // Json::Value listValue = value["data"]["memberList"];
        // int listSize = listValue.size();
        // for(int i = 0; i<listSize; ++i){
        //     std::cout<<"司机名称： "<< listValue[i]["name"].asString() << std::endl;
        //     std::cout<<"vehcileCode ： "<< listValue[i]["vehcileCode"].asString() << std::endl;
        //     std::cout<<"deviceCode： "<< listValue[i]["deviceCode"].asString() << std::endl;

        //     devIDstr = listValue[i]["vehcileCode"].asString();
        //     cluList.push_back(devIDstr);
        // }

        std::cout << "name = " << value["data"]["name"].asString() << std::endl;
        std::cout << "id = " << value["data"]["id"].asString() << std::endl;
        std::cout << "type = " << value["data"]["type"].asInt() << std::endl;
        cluIDstr = value["data"]["id"].asString();

        // cluInfoMap[devIDstr] = cluIDstr;
	}
}

bool sharHttpSer::request_status()
{
    if(804000 == _code) return true;
    return false;
}

bool sharHttpSer::POST_update(std::string device, int state)
{
    _code = 0;
    CURL* curl = nullptr;
	CURLcode res;
	char postData[128] = {'\0'};
	sprintf(postData, "{\"deviceCode\":\"%s\",\"state\":%d}", device.data(), state);
	// printf("-----------postData---------- %s\n", postData);

    strResponse.clear();
    curl = curl_easy_init();
	if (curl)
	{	
		curl_easy_setopt(curl, CURLOPT_URL, UPURL);
        curl_easy_setopt(curl,CURLOPT_POST, 1);
        struct curl_slist *plist = curl_slist_append(NULL, "Content-Type:application/json;charset=UTF-8");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, plist);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, BufferWriterFunc);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &strResponse);
		
		res = curl_easy_perform(curl);
		if (res != CURLE_OK)
		{
			std::cout << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
		}
		else
		{
			// std::cout << "curl_easy_perform() success." << std::endl;
			std::cout << "strResponse is: " << strResponse << std::endl;

			// strResponse.clear();
			// strResponse = "{\"msg\": \"请求成功\",\"code\": 804000, \"data\": {\"port\": 9192, \"ip\": \"127.0.0.1\"}}";
            Json_analyze2();
		}

        curl_slist_free_all(plist);
		curl_easy_cleanup(curl);

	}

	if(804000 == _code) return true;
    return false;
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

void sharHttpSer::Json_analyze2()
{
	Json::Reader reader;
	Json::Value value;

	if (reader.parse(strResponse, value)){
		_code = value["code"].asInt();

		std::cout << "code = " << value["code"].asInt() << std::endl;
		// std::cout << "authorizeCode = " << value["data"]["authorizeCode"] << std::endl;
		// std::string _deviceCode = value["data"]["deviceCode"].asString();
		// std::cout << "vehicleCode = " << _deviceCode << std::endl;
		std::cout << "msg = " << value["msg"].asString() << std::endl;
	}
}
