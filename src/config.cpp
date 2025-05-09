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
				req = config->FirstChildElement("serverip");
				if(req == NULL) {
                    printf("get serverip fail!\n");
                    break;
                }
				strncpy(s_config.serverip, req->GetText(), sizeof(s_config.serverip) - 1);
 
				req = config->FirstChildElement("serverport");
				if(req == NULL || req->QueryIntText(&s_config.port) != XML_SUCCESS)
					break;

				req = config->FirstChildElement("UrlKey");
				if(req == NULL) break;
				strncpy(s_config.UrlKey, req->GetText(), sizeof(s_config.UrlKey) - 1);

				req = config->FirstChildElement("urlDNS");
				if(req == NULL)
					break;
				strncpy(s_config.urlDNS, req->GetText(), sizeof(s_config.urlDNS) - 1);

				load = 1;

			} while(0);

		}
		if(!load)
			return -1;
	}
	*cfg = s_config;
	return 0;
}
