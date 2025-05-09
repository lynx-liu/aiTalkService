#ifndef _SHAR_TASK_MEMPOOL_H
#define _SHAR_TASK_MEMPOOL_H
#include <list>
#include "shar_RTPServerEngine.h"

//sharTalk JT/T1078
class Task_S
{
    public:
        Task_S(CONFIG config_, int fd);
        ~Task_S();
        
        void init_fd(int fd);
        bool run();
    private:
        EngineServerJTT* engine_;
};

class taskmempool_S
{
public:
    taskmempool_S(CONFIG config_, int count);
    ~taskmempool_S();
    Task_S* get_memblock(int fd);
    void regression_mempool(Task_S* task_);
private:
    void mempool_init(int memcount);
    void mempool_release();
private:
    CONFIG config;
    Task_S* task;
    Mutex _mutex;
    std::list<Task_S*> memList;

    // int num;
    // int cout;
    // int input;
};

#endif
