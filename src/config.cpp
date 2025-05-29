#include "config.h"
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
		XMLElement* config = NULL;
		if(XML_SUCCESS == doc.LoadFile("config.xml") && (config = doc.FirstChildElement("config")) != NULL){
			XMLElement* req = NULL;

			do{
				req = config->FirstChildElement("httpserver");
				if(req == NULL) {
                    printf("get httpserver fail!\n");
                    break;
                }
				strncpy(s_config.httpserver, req->GetText(), sizeof(s_config.httpserver) - 1);
 
				req = config->FirstChildElement("audioport");
				if(req == NULL || req->QueryIntText(&s_config.audioport) != XML_SUCCESS)
					break;

				load = 1;

			} while(0);

		}
		if(!load) {
			strncpy(s_config.httpserver, "https://wechat.che-mi.net", sizeof(s_config.httpserver));
			s_config.audioport = 9191;
		}
	}
	*cfg = s_config;
	return 0;
}
