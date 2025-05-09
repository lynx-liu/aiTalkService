#ifndef _VIDEO_WORK_POOL_H_
#define _VIDEO_WORK_POOL_H_
#include "RTPServerEngine.h"
#include "config.h"
#include "lock.h"
#include "taskmempool.h"

class videoWorkpool
{
public:
    videoWorkpool(CONFIG config_);
    ~videoWorkpool();
    void init_threadpool2(int threadNum);
    void add_device_detonate_event2(int fd);

private:
    void create_work2();
    bool create_task2();
    static void* _work_thread2(void* This);
    static void* _task_thread2(void* This);
    Task* get_device_detonate_event2();
    int get_event_num2();
    int get_live_thread_num();
    void add_live_thread_num();
    void decrease_live_thread_num();
    void add_busy_thr_num();
    void task_run();
    void add_to_queue2(Task*);
    void exit_queue2();
private:
    CONFIG          _config;
    int             event_num;
    taskmempool*    taskmempoolptr;
    queue<Task*>    event_queue2;
    pthread_t       pid2;
    Mutex           threadMutex;
    Mutex           event_mutex2;
    Cond            cond2;
    int             live_thr_num;
    int             busy_thr_num;
    Task*           task;             
};

#endif