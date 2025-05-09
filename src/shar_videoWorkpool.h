#ifndef _SHAR_VIDEO_WORK_POOL_H_
#define _SHAR_VIDEO_WORK_POOL_H_
#include "config.h"
#include "lock.h"
#include "shar_taskmempool.h"


class videoWorkpool_S
{
public:
    videoWorkpool_S(CONFIG config_, int threNum);
    ~videoWorkpool_S();
    void add_device_detonate_event2(int fd);

private:
    void init_threadpool2(int threadNum);
    void create_work2();
    bool create_task2();
    static void* _work_thread2(void* This);
    static void* _task_thread2(void* This);
    Task_S* get_device_detonate_event2();
    int get_event_num2();
    int get_live_thread_num();
    void add_live_thread_num();
    void decrease_live_thread_num();
    void add_busy_thr_num();
    void task_run();
    void add_to_queue2(Task_S*);
    void exit_queue2();

private:
    CONFIG          _config;
    int             event_num;
    taskmempool_S*    taskmempoolptr;
    queue<Task_S*>    event_queue2;
    pthread_t       pid2;
    Mutex           threadMutex;
    Mutex           event_mutex2;
    Cond            cond2;
    int             live_thr_num;
    int             busy_thr_num;
    Task_S*           task;     
};

#endif