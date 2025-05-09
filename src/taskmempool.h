#ifndef TASK_MEMPOOL_H
#define TASK_MEMPOOL_H
#include <list>
#include "RTPServerEngine.h"

// yue1078
class Task
{
    public:
        Task(CONFIG config_, int fd);
        ~Task();
        
        void init_fd(int fd);
        bool run();
    // private:
        CRTPServerEngine* engine_;
};

class taskmempool
{
public:
    taskmempool(CONFIG config_, int count);
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
    // std::list<Task*>::iterator iter;

    // int num;
    // int cout;
    // int input;
};

struct epollevent
{
    int   fd;
    int   type;
    Task* task;
};

class eventmempool
{
public:
    eventmempool(CONFIG config_, int count);
    ~eventmempool();
    epollevent* get_memblock(int fd, bool block, int type_);
    void regression_mempool(epollevent* eventptr, bool block);
private:
    void mempool_init(int memcount);
    void mempool_release();
private:
    Mutex _mutex;
    CONFIG config;
    Task*        task;
    epollevent*  eventptr;
    taskmempool* taskmempoolptr;
    std::list<epollevent*> memList;

    // int num;
    // int cout;
};


#endif