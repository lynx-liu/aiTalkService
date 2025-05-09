#include "shar_taskmempool.h"

//JT/T1078
Task_S::Task_S(CONFIG config_, int fd)
{
    engine_ = nullptr;
    engine_ = new EngineServerJTT{config_, fd};
}

Task_S::~Task_S()
{
    if(engine_){
        engine_->close_and_free();
        delete engine_;
        engine_ = nullptr;
    }
}

void Task_S::init_fd(int fd)
{
    if(engine_) engine_->init(fd);
}

bool Task_S::run()
{
    if(engine_){
        if(!engine_->ReadAndAnalyzeRTPPack()) {
            engine_->reInit();
            return false;
        }
        return true;
    }
    return false;
}

taskmempool_S::taskmempool_S(CONFIG config_, int count)
{
    // num  = 0;
    // cout = 0;
    // input = 0;
    config = config_;
    mempool_init(count);
}

taskmempool_S::~taskmempool_S()
{
    mempool_release();
}

void taskmempool_S::mempool_init(int memcount)
{
    int i = 0;
    _mutex.mutex_lock();
    for(int index = 0; index< memcount; index++){
        task = nullptr;
        task = new Task_S{config, 0};
        if(task) memList.push_back(task);
    //     ++num;
    // LOG(INFO)<<"******************** "<< num;
    }
    _mutex.mutex_unlock();
}

Task_S* taskmempool_S::get_memblock(int fd)
{
    task = nullptr;
    _mutex.mutex_lock();
    if(!memList.empty()){
        task = memList.front();
        memList.pop_front();
        // ++cout;
        // LOG(INFO)<< "+++++++++++++++++++++++++ "<< cout;
    }else{
        task = new Task_S{config, 0};
    //     ++num;
    // LOG(INFO)<<"******************** "<< num;
    }
    _mutex.mutex_unlock();

    if(task) task->init_fd(fd);
    return task;
}

void taskmempool_S::regression_mempool(Task_S* task_)
{
    _mutex.mutex_lock();
    memList.push_back(task_);
    _mutex.mutex_unlock();

    // ++input;
    // LOG(INFO)<< "____________________________________input = "<< input;
}

void taskmempool_S::mempool_release()
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