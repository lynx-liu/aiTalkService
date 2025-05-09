#include "videoWorkpool.h"

videoWorkpool::videoWorkpool(CONFIG config_):
_config(config_),
event_num(0)
{
    taskmempoolptr = nullptr;
}

videoWorkpool::~videoWorkpool()
{
    if(nullptr != taskmempoolptr){
        delete taskmempoolptr;
        taskmempoolptr = nullptr;
    }
    exit_queue2();
}

void videoWorkpool::init_threadpool2(int threadNum)
{
    live_thr_num = 0;
    busy_thr_num = 0;
    taskmempoolptr = new taskmempool(_config, 150);
    

    create_work2();
    for(int i = 0; i< threadNum; i++){
        create_task2();
    }
}

void videoWorkpool::add_device_detonate_event2(int fd)
{
    task = nullptr;
    task = taskmempoolptr->get_memblock(fd);
    event_mutex2.mutex_lock();
    event_queue2.push(task);
    event_num++;
    event_mutex2.mutex_unlock();
}

void videoWorkpool::create_work2()
{
    if(pthread_create(&pid2,NULL,_work_thread2,this)!=0){
        // LOG(INFO)<< "create work thread fail";
        exit(1);
    }
}

bool videoWorkpool::create_task2()
{
    pthread_t task_pid;
    if(pthread_create(&task_pid,NULL,_task_thread2,this)!=0){
        // LOG(INFO)<< "create task thread fail";
        return false;
    }
    add_live_thread_num();
    return true;
}

void videoWorkpool::exit_queue2()
{
    int size = 0;
    Task* _task;
    event_mutex2.mutex_lock();
    size = event_queue2.size();
    for(int i = 0; i< size; i++){
        _task = nullptr;
        _task = event_queue2.front();
        event_queue2.pop();
        event_num--;
        delete _task;
    }
    event_mutex2.mutex_unlock();
}

void videoWorkpool::add_live_thread_num()
{
    threadMutex.mutex_lock();
    live_thr_num++;
    threadMutex.mutex_unlock();
}

void videoWorkpool::decrease_live_thread_num()
{
    threadMutex.mutex_lock();
    live_thr_num--;
    threadMutex.mutex_unlock();
}

int videoWorkpool::get_live_thread_num()
{
    int num = 0;
    threadMutex.mutex_lock();
    num = live_thr_num;
    threadMutex.mutex_unlock();
    return num;
}

int videoWorkpool::get_event_num2()
{
    int num = 0;
    event_mutex2.mutex_lock();
    num = event_num;
    event_mutex2.mutex_unlock();
    return num;
}

void videoWorkpool::add_to_queue2(Task* task)
{
    event_mutex2.mutex_lock();
    event_queue2.push(task);
    event_num++;
    event_mutex2.mutex_unlock();
}

Task* videoWorkpool::get_device_detonate_event2()
{
    Task* task = nullptr;
    event_mutex2.mutex_lock();
    if(!event_queue2.empty()){
        task = event_queue2.front();
        event_queue2.pop();
        event_num--;
    }
    event_mutex2.mutex_unlock();
    return task;
}

void* videoWorkpool::_work_thread2(void* This)
{
    pthread_detach(pthread_self());
    videoWorkpool* _this = (videoWorkpool*)This;

    int ReNum;
    int liveNum;
    for(;;)
    {
        usleep(5);
        
        ReNum = _this->get_event_num2();
        if(ReNum > 0){
            liveNum = 0;
            liveNum = _this->get_live_thread_num();
            if(liveNum > 0){
                _this->cond2.signal();
            }
        }
    }
}

void* videoWorkpool::_task_thread2(void* This)
{
    pthread_detach(pthread_self());
    videoWorkpool* _this = (videoWorkpool*)This;

    _this->task_run();
    pthread_exit(NULL);
}

void videoWorkpool::task_run()
{
    Task* task;
    for(;;){
        cond2.wait();
        decrease_live_thread_num();

        task = nullptr;
        task = get_device_detonate_event2();
        if(task){
            if(task->run()){
                add_to_queue2(task);
            }else {
                taskmempoolptr->regression_mempool(task);
            }
        }

        add_live_thread_num();
    }
}