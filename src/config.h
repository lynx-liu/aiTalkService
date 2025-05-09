#include "StreDataType.h"

#ifndef _CONFIG_H_
#define _CONFIG_H_
typedef struct config
{   
    char serverip[64];
    int  prot;
    int  wprot;
    char UrlKey[128];
    char urlDNS[128];
    int  httpserport;
}CONFIG;

int get_config(CONFIG* cfg);


#endif