#include "lock.h"

Sem::Sem()
{
    if(sem_init(&sem,0,0)!=0)                      //信号量的初始值和和基于内存的信号量
        cerr<<"sem init error."<<endl;
}

Sem::~Sem()
{
    sem_destroy(&sem);
}

bool Sem::wait()
{
    return sem_wait(&sem)==0?true:false;
}

bool Sem::post()
{
    return sem_post(&sem)==0?true:false;
}




Mutex::Mutex()
{
    if(pthread_mutex_init(&mutex,NULL)!=0)              //可用PTHRAD_MUTEX_INITIALIZER宏初始化
        cerr<<"mutex init error"<<endl;
}

Mutex::~Mutex()
{
    pthread_mutex_destroy(&mutex);
}

bool Mutex::mutex_lock()
{
    return pthread_mutex_lock(&mutex)==0?true:false;
}

bool Mutex::mutex_unlock()
{
    return pthread_mutex_unlock(&mutex)==0?true:false;
}



Cond::Cond()
{
    if(pthread_mutex_init(&mutex,NULL)!=0)
    {
        cerr<<"Cond mutex init error"<<endl;
        exit(0);
    }
    if(pthread_cond_init(&cond,NULL)!=0)
    {
        cerr<<"Cond cond init error"<<endl;
        pthread_mutex_destroy(&mutex);
        exit(0);
    }
}

Cond::~Cond()
{
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
}

bool Cond::wait()
{
    int rs=0;
    pthread_mutex_lock(&mutex);
    rs=pthread_cond_wait(&cond,&mutex);
    pthread_mutex_unlock(&mutex);
    return rs==0?true:false;
}

bool Cond::signal()
{
    return pthread_cond_signal(&cond)==0?true:false;
}

bool Cond::broadcast()
{
    return pthread_cond_broadcast(&cond);
}




TimeCond::TimeCond()
{
    if(pthread_mutex_init(&mutex,NULL)!=0)
    {
        cerr<<"Cond mutex init error"<<endl;
        exit(0);
    }
    if(pthread_cond_init(&cond,NULL)!=0)
    {
        cerr<<"Cond cond init error"<<endl;
        pthread_mutex_destroy(&mutex);
        exit(0);
    }

    timeout_ms = 2;
}

TimeCond::~TimeCond()
{
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
}

bool TimeCond::timedwait()
{
    int rs=0;
    gettimeofday(&now, NULL);
    nsec = now.tv_usec * 1000 + (timeout_ms % 1000) * 1000000;
    abstime.tv_nsec=nsec % 1000000000;
    pthread_mutex_lock(&mutex);
    rs=pthread_cond_timedwait(&cond,&mutex, &abstime);
    pthread_mutex_unlock(&mutex);
    return rs==0?true:false;
}

bool TimeCond::signal()
{
    int rs = 0;
    pthread_mutex_lock(&mutex);
    rs = pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    return rs==0?true:false;
}

bool TimeCond::broadcast()
{
    int rs = 0;
    pthread_mutex_lock(&mutex);
    rs = pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);
    return rs==0?true:false;
}