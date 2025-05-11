#include "taskmempool.h"

//BCDSIMLength==10:粤标1078，BCDSIMLength==6：JTT1078
Task::Task(CONFIG config_, uint8_t BCDSIMLength)
{
    engine_ = new CRTPServerEngine{config_, BCDSIMLength};
}

Task::~Task()
{
    if(engine_){
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
    if(engine_){
        if(!engine_->ReadAndAnalyzeRTPPack()) {
            engine_->reInit();
            return false;
        }
        return true;
    }
    return false;
}

taskmempool::taskmempool(CONFIG config_, uint8_t BCDSIMLength, int count)
{
    config = config_;
    m_BCDSIMLength = BCDSIMLength;
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
        task = new Task{config, m_BCDSIMLength};
        if(task) memList.push_back(task);
    }
    _mutex.mutex_unlock();
}

Task* taskmempool::get_memblock(int fd)
{
    _mutex.mutex_lock();
    if(!memList.empty()){
        task = memList.front();
        memList.pop_front();
    }else{
        task = new Task{config, m_BCDSIMLength};
    }
    _mutex.mutex_unlock();

    if(task) task->init_fd(fd);
    return task;
}

void taskmempool::regression_mempool(Task* task)
{
    _mutex.mutex_lock();
    memList.push_back(task);
    _mutex.mutex_unlock();
}

void taskmempool::mempool_release()
{
    int num = 0;
    _mutex.mutex_lock();
    num = memList.size();
    if(num>0){
        for(int index = 0; index < num; index++){
            task = memList.front();
            delete task;
            memList.pop_front();
        }
    }
    _mutex.mutex_unlock();
}
