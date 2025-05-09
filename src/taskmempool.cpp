#include "taskmempool.h"

//yue1078
Task::Task(CONFIG config_, int fd)
{
    engine_ = nullptr;
    engine_ = new CRTPServerEngine{config_, fd};
}

Task::~Task()
{
    if(engine_ != nullptr){
        engine_->close_and_free();
        delete engine_;
        engine_ = nullptr;
    }
}

void Task::init_fd(int fd)
{
    if(engine_) engine_->init(fd);
}

bool Task::run()
{
    if(engine_ != nullptr){
            if(!engine_->ReadAndAnalyzeRTPPack()) {
                engine_->reInit();
                return false;
            }
        return true;
    }
    return false;
}

taskmempool::taskmempool(CONFIG config_, int count)
{
    // num  = 0;
    // cout = 0;
    // input = 0;
    config = config_;
    mempool_init(count);
}

taskmempool::~taskmempool()
{
    mempool_release();
}

void taskmempool::mempool_init(int memcount)
{
    int i = 0;
    _mutex.mutex_lock();
    for(int index = 0; index< memcount; index++){
        task = nullptr;
        task = new Task{config, 0};
        memList.push_back(task);
    //     ++num;
    // LOG(INFO)<<"******************** "<< num;
    }
    _mutex.mutex_unlock();
}

Task* taskmempool::get_memblock(int fd)
{
    task = nullptr;
    _mutex.mutex_lock();
    if(!memList.empty()){
        task = memList.front();
        memList.pop_front();
        // ++cout;
        // LOG(INFO)<< "+++++++++++++++++++++++++ "<< cout;
    }else{
        task = new Task{config, 0};
    //     ++num;
    // LOG(INFO)<<"******************** "<< num;
    }
    _mutex.mutex_unlock();

    task->init_fd(fd);
    return task;
}

void taskmempool::regression_mempool(Task* task)
{
    _mutex.mutex_lock();
    memList.push_back(task);
    _mutex.mutex_unlock();

    // ++input;
    // LOG(INFO)<< "____________________________________input = "<< input;
}

void taskmempool::mempool_release()
{
    int num = 0;
    _mutex.mutex_lock();
    num = memList.size();
    if(num>0){
        for(int index = 0; index < num; index++){
            task = nullptr;
            task = memList.front();
            delete task;
            memList.pop_front();
        }
    }
    _mutex.mutex_unlock();
}

//eventmempool

eventmempool::eventmempool(CONFIG config_, int count):
config(config_)
{
    // num  = 0;
    // cout = 0;
    mempool_init(count);
}

eventmempool::~eventmempool()
{
    if(nullptr != taskmempoolptr){
        delete taskmempoolptr;
        taskmempoolptr = nullptr;
    }

    mempool_release();
}

void eventmempool::mempool_init(int memcount)
{
    taskmempoolptr = new taskmempool(config, memcount);

    _mutex.mutex_lock();
    for(int index = 0; index< memcount; index++){
        eventptr = nullptr;
        eventptr = new epollevent();
        memList.push_back(eventptr);
    //     ++num;
    // LOG(INFO)<<"*********event*********** "<< num;
    }
    _mutex.mutex_unlock();
}

epollevent* eventmempool::get_memblock(int fd, bool block, int type_)
{
    if(fd < 0) {
        // LOG(INFO)<< "fd < 0 ERROR!";
        return nullptr;
    }

    eventptr = nullptr;
    _mutex.mutex_lock();
    if(!memList.empty()){
        eventptr = memList.front();
        memList.pop_front();
    }else{
        eventptr = new epollevent();
    //     ++num;
    // LOG(INFO)<<"**********event********** "<< num;
    }
    _mutex.mutex_unlock();

    task = nullptr;
    if(block){
        task = taskmempoolptr->get_memblock(fd);
        // LOG(INFO)<<"^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^";
    }
    
    memset(eventptr, 0, sizeof(epollevent));
    eventptr->fd = fd;
    eventptr->type = type_;
    eventptr->task = task;

    // ++cout;
    // LOG(INFO)<< "++++++++++++event+++++++++++++ "<< cout;
    return eventptr;
}

void eventmempool::regression_mempool(epollevent* eventptr, bool block)
{
    if(block && eventptr->task)
        taskmempoolptr->regression_mempool(eventptr->task);

    _mutex.mutex_lock();
    memList.push_back(eventptr);
    _mutex.mutex_unlock();
}

void eventmempool::mempool_release()
{
    int num = 0;
    _mutex.mutex_lock();
    num = memList.size();
    if(num>0){
        for(int index = 0; index < num; index++){
            eventptr = nullptr;
            eventptr = memList.front();
            delete eventptr;
            memList.pop_front();
        }
    }
    _mutex.mutex_unlock();
}