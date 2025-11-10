#ifndef _VIDEO_WORK_POOL_H_
#define _VIDEO_WORK_POOL_H_
#include "RTPServerEngine.h"
#include "config.h"
#include "lock.h"

class Workpool
{
public:
    Workpool(CONFIG config_, int threNum);
    ~Workpool();
    void add_to_queue(int fd);

private:
    bool create_task();
    static void* _task_thread(void* This);
    void task_run();

private:
    CONFIG          _config;
    queue<CRTPServerEngine*>    event_queue;
    Mutex           event_mutex;
    Cond            cond;
};

#endif
