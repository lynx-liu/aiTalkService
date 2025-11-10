#include "StreDataType.h"

#ifndef _CONFIG_H_
#define _CONFIG_H_
typedef struct config
{   
    char httpserver[128];
    char aiserver[128];
    int  audioport;
    int wsport;
}CONFIG;

int get_config(CONFIG* cfg);


#endif