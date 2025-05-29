#include <unistd.h>
#include "workpool.h"

Workpool::Workpool(CONFIG config_, int threNum):
_config(config_),
event_num(0)
{
    init_threadpool(threNum);
}

Workpool::~Workpool()
{
    event_mutex.mutex_lock();
    int size = event_queue.size();
    for(int i = 0; i< size; i++){
        CRTPServerEngine* engine = event_queue.front();
        event_queue.pop();
        event_num--;
        delete engine;
    }
    event_mutex.mutex_unlock();
}

void Workpool::init_threadpool(int threadNum)
{
    live_thr_num = 0;
    create_work();
    for(int i = 0; i< threadNum; i++){
        create_task();
    }
}

void Workpool::add_device_detonate_event(int fd)
{
    event_mutex.mutex_lock();
    CRTPServerEngine* engine = new CRTPServerEngine{fd, _config};
    if(engine) {
        event_queue.push(engine);
        event_num++;
    }
    event_mutex.mutex_unlock();
}

void Workpool::create_work()
{
    if(pthread_create(&work_pid,NULL,_work_thread,this)!=0){
        printf("create work thread fail\n");
        exit(1);
    }
}

bool Workpool::create_task()
{
    pthread_t task_pid;
    if(pthread_create(&task_pid,NULL,_task_thread,this)!=0){
        printf("create task thread fail\n");
        return false;
    }
    add_live_thread_num();
    return true;
}

void Workpool::add_live_thread_num()
{
    threadMutex.mutex_lock();
    live_thr_num++;
    threadMutex.mutex_unlock();
}

void Workpool::decrease_live_thread_num()
{
    threadMutex.mutex_lock();
    live_thr_num--;
    threadMutex.mutex_unlock();
}

int Workpool::get_live_thread_num()
{
    int num = 0;
    threadMutex.mutex_lock();
    num = live_thr_num;
    threadMutex.mutex_unlock();
    return num;
}

int Workpool::get_event_num()
{
    int num = 0;
    event_mutex.mutex_lock();
    num = event_num;
    event_mutex.mutex_unlock();
    return num;
}

void Workpool::add_to_queue(CRTPServerEngine* engine)
{
    event_mutex.mutex_lock();
    event_queue.push(engine);
    event_num++;
    event_mutex.mutex_unlock();
}

CRTPServerEngine* Workpool::get_device_detonate_event()
{
    CRTPServerEngine* engine = nullptr;
    event_mutex.mutex_lock();
    if(!event_queue.empty()){
        engine = event_queue.front();
        event_queue.pop();
        event_num--;
    }
    event_mutex.mutex_unlock();
    return engine;
}

void* Workpool::_work_thread(void* This)
{
    pthread_detach(pthread_self());
    Workpool* _this = (Workpool*)This;

    int ReNum;
    int liveNum;
    for(;;)
    {
        usleep(5);
        
        ReNum = _this->get_event_num();
        if(ReNum > 0){
            liveNum = 0;
            liveNum = _this->get_live_thread_num();
            if(liveNum > 0){
                _this->cond.signal();
            }
        }
    }
}

void* Workpool::_task_thread(void* This)
{
    pthread_detach(pthread_self());
    Workpool* _this = (Workpool*)This;
    _this->task_run();
    pthread_exit(NULL);
}

void Workpool::task_run()
{
    for(;;){
        cond.wait();
        decrease_live_thread_num();

        CRTPServerEngine* engine = get_device_detonate_event();
        if(engine){
            if(engine->ReadAndAnalyzeRTPPack()) {
                add_to_queue(engine);
            } else {
                engine->reInit();
                delete engine;
            }
        }

        add_live_thread_num();
    }
}
