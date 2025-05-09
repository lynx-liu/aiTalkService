#ifndef TASK_MEMPOOL_H
#define TASK_MEMPOOL_H
#include <list>
#include "RTPServerEngine.h"

// yue1078
class Task
{
    public:
        Task(CONFIG config_, uint8_t BCDSIMLength);
        ~Task();
        
        void init_fd(int fd);
        bool run();
    // private:
        CRTPServerEngine* engine_;
};

class taskmempool
{
public:
    taskmempool(CONFIG config_, uint8_t BCDSIMLength, int count);
    ~taskmempool();
    Task* get_memblock(int fd);
    void regression_mempool(Task* task);
private:
    void mempool_init(int memcount);
    void mempool_release();
private:
    CONFIG config;
    Task* task;
    Mutex _mutex;
    std::list<Task*> memList;

    uint8_t m_BCDSIMLength;
};

#endif
