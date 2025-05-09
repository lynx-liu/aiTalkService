#include "shar_adpcmstatemempool.h"

adpcmState::adpcmState(int initNum)
{
    mempool_init(initNum);
}

adpcmState::~adpcmState()
{
    mempool_release();
}

void adpcmState::mempool_init(int memcount)
{
     for(int index = 0; index< memcount; index++){
        adpcmstatePtr = nullptr;
        adpcmstatePtr = new adpcm_state();
        if(adpcmstatePtr) adpcmStaList.push_back(adpcmstatePtr);
    }
}

adpcm_state* adpcmState::get_memblock()
{
    adpcmstatePtr = nullptr;
    if(!adpcmStaList.empty()){
        adpcmstatePtr = adpcmStaList.front();
        adpcmStaList.pop_front();
    }else{
        adpcmstatePtr = new adpcm_state();
    }

    memset(adpcmstatePtr, 0, sizeof(adpcm_state));
    return adpcmstatePtr;
}

void adpcmState::callback_install(adpcm_state* adpcmStae)
{
    if(adpcmStae) {
        memset(adpcmStae, 0, sizeof(adpcm_state));
        adpcmStaList.push_back(adpcmStae);
    }
}

void adpcmState::mempool_release()
{
    int num = 0;
    num = adpcmStaList.size();
    if(num>0){
        for(int index = 0; index < num; index++){
            adpcmstatePtr = nullptr;
            adpcmstatePtr = adpcmStaList.front();
            delete adpcmstatePtr;
            adpcmStaList.pop_front();
        }
    }
}

