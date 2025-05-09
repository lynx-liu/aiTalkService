#ifndef _SHAR_APCMPSTATEMEMPOOL_H
#define _SHAR_APCMPSTATEMEMPOOL_H
#include <list>
#include "StreDataType.h"

class adpcmState
{
public:
    adpcmState(int initNum);
    ~adpcmState();
    adpcm_state* get_memblock();
    void callback_install(adpcm_state* adpcmStae);
private:
    void mempool_init(int memcount);
    void mempool_release();

private:
    // Mutex mutex;
    adpcm_state* adpcmstatePtr;
    std::list<adpcm_state*> adpcmStaList;
};





#endif