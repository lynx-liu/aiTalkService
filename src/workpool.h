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
    void add_device_detonate_event(int fd);

private:
    void init_threadpool(int threadNum);
    void create_work();
    bool create_task();
    static void* _work_thread(void* This);
    static void* _task_thread(void* This);
    CRTPServerEngine* get_device_detonate_event();
    int get_event_num();
    int get_live_thread_num();
    void add_live_thread_num();
    void decrease_live_thread_num();
    void task_run();
    void add_to_queue(CRTPServerEngine*);

private:
    CONFIG          _config;
    int             event_num;
    queue<CRTPServerEngine*>    event_queue;
    pthread_t       work_pid;
    Mutex           threadMutex;
    Mutex           event_mutex;
    Cond            cond;
    int             live_thr_num;
};

#endif
