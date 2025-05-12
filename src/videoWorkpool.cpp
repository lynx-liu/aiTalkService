#include "videoWorkpool.h"

videoWorkpool::videoWorkpool(CONFIG config_, uint8_t BCDSIMLength, int threNum):
_config(config_),
event_num(0),
m_BCDSIMLength(BCDSIMLength)
{
    init_threadpool(threNum);
}

videoWorkpool::~videoWorkpool()
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

void videoWorkpool::init_threadpool(int threadNum)
{
    live_thr_num = 0;
    create_work();
    for(int i = 0; i< threadNum; i++){
        create_task();
    }
}

void videoWorkpool::add_device_detonate_event(int fd)
{
    event_mutex.mutex_lock();
    CRTPServerEngine* engine = new CRTPServerEngine{_config, m_BCDSIMLength};
    if(engine) engine->init(fd);
    event_queue.push(engine);
    event_num++;
    event_mutex.mutex_unlock();
}

void videoWorkpool::create_work()
{
    if(pthread_create(&work_pid,NULL,_work_thread,this)!=0){
        printf("create work thread fail\n");
        exit(1);
    }
}

bool videoWorkpool::create_task()
{
    pthread_t task_pid;
    if(pthread_create(&task_pid,NULL,_task_thread,this)!=0){
        printf("create task thread fail\n");
        return false;
    }
    add_live_thread_num();
    return true;
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

int videoWorkpool::get_event_num()
{
    int num = 0;
    event_mutex.mutex_lock();
    num = event_num;
    event_mutex.mutex_unlock();
    return num;
}

void videoWorkpool::add_to_queue(CRTPServerEngine* engine)
{
    event_mutex.mutex_lock();
    event_queue.push(engine);
    event_num++;
    event_mutex.mutex_unlock();
}

CRTPServerEngine* videoWorkpool::get_device_detonate_event()
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

void* videoWorkpool::_work_thread(void* This)
{
    pthread_detach(pthread_self());
    videoWorkpool* _this = (videoWorkpool*)This;

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

void* videoWorkpool::_task_thread(void* This)
{
    pthread_detach(pthread_self());
    videoWorkpool* _this = (videoWorkpool*)This;
    _this->task_run();
    pthread_exit(NULL);
}

void videoWorkpool::task_run()
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
