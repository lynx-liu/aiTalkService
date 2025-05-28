#include <pthread.h>
#include <map>
#include "audioType.h"
#include "lock.h"

typedef std::map<std::string,  audioType> AUDIO_TYPE_INFO_MAP;
AUDIO_TYPE_INFO_MAP audiTypeInfo;
AUDIO_TYPE_INFO_MAP::iterator iter;
pthread_mutex_t Audio_Type_Mutex = PTHREAD_MUTEX_INITIALIZER;

bool add_audio_type_info(std::string sim,audioType AudtypeInfo)
{
    pthread_mutex_lock(&Audio_Type_Mutex);
    audiTypeInfo[sim] = AudtypeInfo;
    pthread_mutex_unlock(&Audio_Type_Mutex);
    return true;
}

bool get_audio_type_info(std::string sim,audioType& audioInfo)
{
    pthread_mutex_lock(&Audio_Type_Mutex);
    iter = audiTypeInfo.find(sim);
    if(iter != audiTypeInfo.end()){
        audioInfo = audiTypeInfo[sim];
        // audiTypeInfo.erase(iter);
        pthread_mutex_unlock(&Audio_Type_Mutex);
        return true;
    }else {
        pthread_mutex_unlock(&Audio_Type_Mutex);
        return false;
    }
}

void del_audio_type_info(std::string sim)
{
    pthread_mutex_lock(&Audio_Type_Mutex);
    audiTypeInfo.erase(sim);
    pthread_mutex_unlock(&Audio_Type_Mutex);
    printf("----------------- delete audio intfo ----------\n");
}