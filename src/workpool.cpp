#include <unistd.h>
#include "debug.h"
#include "workpool.h"

Workpool::Workpool(CONFIG config_):
_config(config_),
idle_thread_count(0)
{
    create_task();
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
    int queue_size = 0;
    event_mutex.mutex_lock();
    CRTPServerEngine* engine = new CRTPServerEngine{fd, _config};
    if(engine) {
        event_queue.push(engine);
        queue_size = event_queue.size();
    }
    event_mutex.mutex_unlock();

    if(engine) {
        sem.post(); // 信号量：即使所有线程都忙，post 也不会丢失

        int idle_threads = 0;
        state_mutex.mutex_lock();
        idle_threads = idle_thread_count;
        state_mutex.mutex_unlock();

        // 自动扩容：当待处理任务多于空闲线程时，创建新线程
        if(queue_size > idle_threads) {
            create_task();
        }
    }
}

bool Workpool::create_task()
{
    pthread_t task_pid;
    if(pthread_create(&task_pid,NULL,_task_thread,this)!=0){
        printf("\n%screate task thread fail", getNowTime().data());
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
        state_mutex.mutex_lock();
        ++idle_thread_count;

        // 空闲线程超过1时，主动释放当前线程，保持最多1个空闲线程
        if(idle_thread_count > 1){
            --idle_thread_count;
            state_mutex.mutex_unlock();
            break;
        }
        state_mutex.mutex_unlock();

        sem.wait(); // 信号量：每次 post 对应一次 wait，不会丢失

        state_mutex.mutex_lock();
        if(idle_thread_count > 0){
            --idle_thread_count;
        }
        state_mutex.mutex_unlock();

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
