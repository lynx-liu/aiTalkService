#include "shar_videoWorkpool.h"

videoWorkpool_S::videoWorkpool_S(CONFIG config_, int threNum):
_config(config_),
event_num(0)
{
    taskmempoolptr = nullptr;
    init_threadpool2(threNum);
}

videoWorkpool_S::~videoWorkpool_S()
{
    if(taskmempoolptr){
        delete taskmempoolptr;
        taskmempoolptr = nullptr;
    }
    exit_queue2();
}

void videoWorkpool_S::init_threadpool2(int threadNum)
{
    live_thr_num = 0;
    busy_thr_num = 0;
    taskmempoolptr = new taskmempool_S(_config, 50);
    if(!taskmempoolptr) printf("%s:%s:%d new taskmempool == nullptr\n",__FILE__,__FUNCTION__,__LINE__);
    
    create_work2();
    for(int i = 0; i< threadNum; i++){
        create_task2();
    }
}

void videoWorkpool_S::add_device_detonate_event2(int fd)
{
    task = nullptr;
    task = taskmempoolptr->get_memblock(fd);
    event_mutex2.mutex_lock();
    event_queue2.push(task);
    event_num++;
    event_mutex2.mutex_unlock();
}

void videoWorkpool_S::create_work2()
{
    if(pthread_create(&pid2,NULL,_work_thread2,this)!=0){
        // LOG(INFO)<< "create work thread fail";
        exit(1);
    }
}

bool videoWorkpool_S::create_task2()
{
    pthread_t task_pid;
    if(pthread_create(&task_pid,NULL,_task_thread2,this)!=0){
        // LOG(INFO)<< "create task thread fail";
        return false;
    }
    add_live_thread_num();
    return true;
}

void videoWorkpool_S::exit_queue2()
{
    int size = 0;
    Task_S* _task;
    event_mutex2.mutex_lock();
    size = event_queue2.size();
    for(int i = 0; i< size; i++){
        _task = nullptr;
        _task = event_queue2.front();
        event_queue2.pop();
        event_num--;
        if(_task) delete _task;
    }
    event_mutex2.mutex_unlock();
}

void videoWorkpool_S::add_live_thread_num()
{
    threadMutex.mutex_lock();
    live_thr_num++;
    threadMutex.mutex_unlock();
}

void videoWorkpool_S::decrease_live_thread_num()
{
    threadMutex.mutex_lock();
    live_thr_num--;
    threadMutex.mutex_unlock();
}

int videoWorkpool_S::get_live_thread_num()
{
    int num = 0;
    threadMutex.mutex_lock();
    num = live_thr_num;
    threadMutex.mutex_unlock();
    return num;
}

int videoWorkpool_S::get_event_num2()
{
    int num = 0;
    event_mutex2.mutex_lock();
    num = event_num;
    event_mutex2.mutex_unlock();
    return num;
}

void videoWorkpool_S::add_to_queue2(Task_S* task)
{
    event_mutex2.mutex_lock();
    event_queue2.push(task);
    event_num++;
    event_mutex2.mutex_unlock();
}

Task_S* videoWorkpool_S::get_device_detonate_event2()
{
    Task_S* task = nullptr;
    event_mutex2.mutex_lock();
    if(!event_queue2.empty()){
        task = event_queue2.front();
        event_queue2.pop();
        event_num--;
    }
    event_mutex2.mutex_unlock();
    return task;
}

void* videoWorkpool_S::_work_thread2(void* This)
{
    pthread_detach(pthread_self());
    videoWorkpool_S* _this = (videoWorkpool_S*)This;

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

void* videoWorkpool_S::_task_thread2(void* This)
{
    pthread_detach(pthread_self());
    videoWorkpool_S* _this = (videoWorkpool_S*)This;

    _this->task_run();
    pthread_exit(NULL);
}

void videoWorkpool_S::task_run()
{
    Task_S* task;
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