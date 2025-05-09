#ifndef LOCK_H
#define LOCK_H
#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <sys/time.h>
using namespace std;

class Sem
{
    private:
        sem_t sem;

    public:
        Sem();
        ~Sem();
        bool wait();
        bool post();
};

//互斥类
class Mutex
{
    public:
        pthread_mutex_t mutex;

    public:
        Mutex();
        ~Mutex();
        bool mutex_lock();
        bool mutex_unlock();
};

//条件变量的类
class Cond
{
    private:
        pthread_mutex_t mutex;
        pthread_cond_t cond;

    public:
        Cond();
        ~Cond();
        bool wait();
        bool signal();
        bool broadcast();
};

//超时条件变量的类
class TimeCond
{
    public:
        TimeCond();
        ~TimeCond();
        bool timedwait();
        bool signal();
        bool broadcast();
    
    private:
        pthread_mutex_t mutex;
        pthread_cond_t cond;

        struct timespec abstime;
	    struct timeval now;
	    long nsec;
	    long timeout_ms;
};

#endif