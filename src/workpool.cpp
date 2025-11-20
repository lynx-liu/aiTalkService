#include <unistd.h>
#include "workpool.h"

Workpool::Workpool(CONFIG config_, int threNum):
_config(config_)
{
    for(int i = 0; i< threNum; i++){
        create_task();
    }
}

Workpool::~Workpool()
{
    event_mutex.mutex_lock();
    int size = event_queue.size();
    for(int i = 0; i< size; i++){
        CRTPServerEngine* engine = event_queue.front();
        event_queue.pop();
        delete engine;
    }
    event_mutex.mutex_unlock();
}

void Workpool::add_to_queue(int fd)
{
    event_mutex.mutex_lock();
    CRTPServerEngine* engine = new CRTPServerEngine{fd, _config};
    if(engine) {
        event_queue.push(engine);
        cond.signal();
    }
    event_mutex.mutex_unlock();
}

bool Workpool::create_task()
{
    pthread_t task_pid;
    if(pthread_create(&task_pid,NULL,_task_thread,this)!=0){
        printf("\ncreate task thread fail");
        return false;
    }
    return true;
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

        CRTPServerEngine* engine = nullptr;
        event_mutex.mutex_lock();
        if(!event_queue.empty()){
            engine = event_queue.front();
            event_queue.pop();
        }
        event_mutex.mutex_unlock();

        if(engine){
            engine->ReadAndAnalyzeRTPPack();
            delete engine;
        }
    }
}
