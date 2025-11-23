#include "config.h"
#include "debug.h"
#include "tinyxml2.h"

using namespace tinyxml2;
static CONFIG s_config = {0};
static int load = 0;
using namespace tinyxml2;
int get_config(CONFIG* cfg)
{
	if(!load)
	{
		XMLDocument doc;
		XMLError xmlError = doc.LoadFile("config.xml");
		if(XML_SUCCESS == xmlError){
			XMLElement* config = doc.FirstChildElement("config");

			do{
				if(config==NULL)
					break;
					
				XMLElement* req = config->FirstChildElement("httpserver");
				if(req == NULL) {
                    printf("%sget httpserver fail!\n", getNowTime().data());
                    break;
                }
				strncpy(s_config.httpserver, req->GetText(), sizeof(s_config.httpserver) - 1);
 
				req = config->FirstChildElement("aiserver");
				if(req == NULL) {
                    printf("%sget aiserver fail!\n", getNowTime().data());
                    break;
                }
				strncpy(s_config.aiserver, req->GetText(), sizeof(s_config.aiserver) - 1);

				req = config->FirstChildElement("audioport");
				if(req == NULL || req->QueryIntText(&s_config.audioport) != XML_SUCCESS)
					break;

				req = config->FirstChildElement("wsport");
				if(req == NULL || req->QueryIntText(&s_config.wsport) != XML_SUCCESS)
					break;

				load = 1;

			} while(0);

		}
		if(!load) {
			strncpy(s_config.httpserver, "https://wechat.che-mi.net", sizeof(s_config.httpserver));
			strncpy(s_config.aiserver, "https://wechat.che-mi.net", sizeof(s_config.aiserver));
			s_config.audioport = 9191;
			s_config.wsport = 9000;
		}

		printf("%shttpserver: %s\n", getNowTime().data(), s_config.httpserver);
		printf("%saiserver: %s\n", getNowTime().data(), s_config.aiserver);
	}
	*cfg = s_config;
	return 0;
}
