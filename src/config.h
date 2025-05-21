#include "StreDataType.h"

#ifndef _CONFIG_H_
#define _CONFIG_H_
typedef struct config
{   
    char httpserver[128];
    int  audioport;
    int bcdlenght;
    char UrlKey[128];
    char urlDNS[128];
}CONFIG;

int get_config(CONFIG* cfg);


#endif