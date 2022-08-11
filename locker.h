#ifndef LOCKER_H
#define LOCKER_H

#include <pthread.h>
#include <exception>
#include <semaphore.h>

class locker{

public:
    locker(){

        if(pthread_mutex_init(&mutex, nullptr) != 0){

            throw std::exception();
        }
    }

    bool lock(){

        return pthread_mutex_lock(&mutex) == 0;
    }

    bool unlock(){

        return pthread_mutex_unlock(&mutex) == 0;
    }

    pthread_mutex_t* get(){

        return &mutex;
    }
    ~locker(){

        pthread_mutex_destroy(&mutex);
    }

private:
    pthread_mutex_t mutex;

};


class cond{

public:

    cond(){

        if(pthread_cond_init(&m_cond, nullptr) != 0){

            throw std::exception();
        }
    }

    bool wait(pthread_mutex_t* mutex){

        return pthread_cond_wait(&m_cond, mutex) == 0;
    }

    bool timedwait(pthread_mutex_t* mutex, struct timespec* timesp){

        return pthread_cond_timedwait(&m_cond, mutex, timesp) == 0;
    }

    bool signalwait(){

        return pthread_cond_signal(&m_cond) == 0;
    }

    bool broadcast(){

        return pthread_cond_broadcast(&m_cond) == 0;
    }
    ~cond(){

        pthread_cond_destroy(&m_cond);
    }
private:

    pthread_cond_t m_cond;
};



class sem{

public:

    sem(){

        if(sem_init(&m_sem, 0, 0) != 0){

            throw std::exception();
        }
    }

    bool wait(){

        return sem_wait(&m_sem) == 0;
    }

    bool post(){

        return sem_post(&m_sem) == 0;
    }
    ~sem(){

        sem_destroy(&m_sem);
    }

private:

    sem_t m_sem;
};
#endif