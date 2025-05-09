#include <pthread.h>
#include <sys/socket.h>
#include <map>
#include "audioType.h"
#include "converter.h"
#include "lock.h"

int Index = 0;
pthread_mutex_t IndexMutex = PTHREAD_MUTEX_INITIALIZER;
int UniqueIdentity()
{
    pthread_mutex_lock(&IndexMutex);
    ++Index;
    pthread_mutex_unlock(&IndexMutex);
    return Index;
}
/********/
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

bool get_audio_type_info2(std::map<std::string,  audioType>& _sharType)
{
    // mutex1.mutex_lock();
    pthread_mutex_lock(&Audio_Type_Mutex);
    // for (iter = audiTypeInfo.begin(); iter != audiTypeInfo.end(); ++iter){
    //     _sharType.insert(pair<std::string, audioType>(iter->first, iter->second));
    // }

    _sharType = audiTypeInfo;
    // mutex1.mutex_unlock();
    pthread_mutex_unlock(&Audio_Type_Mutex);
    return true;
}

void del_audio_type_info(std::string sim)
{
    pthread_mutex_lock(&Audio_Type_Mutex);
    // iter = audiTypeInfo.find(sim);
    // if(iter != audiTypeInfo.end())
    //     audiTypeInfo.erase(iter);
    audiTypeInfo.erase(sim);
    pthread_mutex_unlock(&Audio_Type_Mutex);
    printf("----------------- delete audio intfo ----------\n");
}


Mutex umutex;
std::map<std::string, std::string> httpReMap;
void install_deviceID(std::string sim, std::string strID)
{
    umutex.mutex_lock();
    httpReMap[sim] = strID;
    umutex.mutex_unlock();
}

void get_allDeviceID(std::map<std::string, std::string>& deviInfoMap)
{
    umutex.mutex_lock();
    deviInfoMap = httpReMap;
    umutex.mutex_unlock();
}

void delete_deviceID_info(std::string sim)
{
     umutex.mutex_lock();
     httpReMap.erase(sim);
     umutex.mutex_unlock();
}




/************/
typedef std::map<std::string,  int> RECEIVE_AUDIO_CONNECT_INFO_MAP;
RECEIVE_AUDIO_CONNECT_INFO_MAP RecAudioConnInfoMap;
RECEIVE_AUDIO_CONNECT_INFO_MAP::iterator _iter;
pthread_mutex_t Audio_Conn_Mutex = PTHREAD_MUTEX_INITIALIZER;
bool add_audio_connect_info(std::string _sim,int fd)
{
    pthread_mutex_lock(&Audio_Conn_Mutex);
    RecAudioConnInfoMap[_sim] = fd;
    pthread_mutex_unlock(&Audio_Conn_Mutex);
    return true;
}

int get_audio_connect_info(std::string _sim)
{
    int Fd = -1;
    pthread_mutex_lock(&Audio_Conn_Mutex);
    _iter = RecAudioConnInfoMap.find(_sim);
    if(_iter != RecAudioConnInfoMap.end()){
        Fd = RecAudioConnInfoMap[_sim];
        // RecAudioConnInfoMap.erase(_iter);
    }
    pthread_mutex_unlock(&Audio_Conn_Mutex);
    return Fd;
}

void del_audio_connect_info(std::string _sim)
{
    pthread_mutex_lock(&Audio_Conn_Mutex);
    // _iter = RecAudioConnInfoMap.find(_sim);
    // if(_iter != RecAudioConnInfoMap.end())
    //     RecAudioConnInfoMap.erase(_iter);
    RecAudioConnInfoMap.erase(_sim);
    pthread_mutex_unlock(&Audio_Conn_Mutex);
    printf("----------------- delete websocket intfo ----------\n");
}



/************/
typedef std::map<std::string, int> _INPUT_HTTP_REQUEST_MAP_H;
_INPUT_HTTP_REQUEST_MAP_H _InputHttpQuest_h;
_INPUT_HTTP_REQUEST_MAP_H::iterator _iter_h;
pthread_mutex_t _Http_Quest_Mutex = PTHREAD_MUTEX_INITIALIZER;
bool find_http_request(std::string sim_c)
{
    pthread_mutex_lock(&_Http_Quest_Mutex);
    _iter_h = _InputHttpQuest_h.find(sim_c);
    if(_iter_h != _InputHttpQuest_h.end()){
        pthread_mutex_unlock(&_Http_Quest_Mutex);
        return true;
    }
    pthread_mutex_unlock(&_Http_Quest_Mutex);
    return false;
}
bool input_http_quest_info_h(std::string sim_c, int fd)
{
    pthread_mutex_lock(&_Http_Quest_Mutex);
    // _iter_h = _InputHttpQuest_h.find(sim_c);
    // if(_iter_h != _InputHttpQuest_h.end()){
    //     pthread_mutex_unlock(&_Http_Quest_Mutex);
    //     return false;
    // }
    _InputHttpQuest_h[sim_c] = fd;
    pthread_mutex_unlock(&_Http_Quest_Mutex);
    return true;
}

int get_http_quest_info_h(std::string sim_c)
{
    int FD = -1;
    pthread_mutex_lock(&_Http_Quest_Mutex);
    _iter_h = _InputHttpQuest_h.find(sim_c);
    if(_iter_h != _InputHttpQuest_h.end()){
        FD = _InputHttpQuest_h[sim_c];
    }
    pthread_mutex_unlock(&_Http_Quest_Mutex);
    return FD;
}

bool del_http_quest_info_h(std::string sim_c)
{
    pthread_mutex_lock(&_Http_Quest_Mutex);
    _iter_h = _InputHttpQuest_h.find(sim_c);
    if(_iter_h != _InputHttpQuest_h.end()){
        _InputHttpQuest_h.erase(_iter_h);
    }
    pthread_mutex_unlock(&_Http_Quest_Mutex);
    return true;
}


/*************插入HTTP视频请求信息*/ 
Mutex mutex;
std::map<std::string, Cnvt::CConverter*> requestFdMap;
std::map<std::string, Cnvt::CConverter*>::iterator converIter;

bool input_info(std::string sim_c, Cnvt::CConverter* _cnvtOBJ)
{
    mutex.mutex_lock();
    requestFdMap[sim_c] = _cnvtOBJ;
    mutex.mutex_unlock();
    return true;
}

Cnvt::CConverter* get_input_info(std::string sim_c)
{
    Cnvt::CConverter* cnvtOBJ = nullptr;
    mutex.mutex_lock();
    converIter = requestFdMap.find(sim_c);
    if(converIter != requestFdMap.end()){
        cnvtOBJ = requestFdMap[sim_c];
    }
    mutex.mutex_unlock();
    return cnvtOBJ;
}

void delete_input_info(std::string sim_c)
{
    mutex.mutex_lock();
    converIter = requestFdMap.find(sim_c);
    if(converIter != requestFdMap.end()){
        requestFdMap.erase(converIter);
    }
    mutex.mutex_unlock();
}
