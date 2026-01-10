#pragma once

#include <pthread.h>
#include "InterruptorArranque.h"

class HiloIntArranque {
private:
    InterruptorArranque* interruptor_;
    bool* running_;
    pthread_mutex_t* mtx_;
    pthread_t thread_;
    double frequency_;
    
    void run();
    static void* threadFunc(void* arg);

public:
    HiloIntArranque(InterruptorArranque* interruptor, bool* running, pthread_mutex_t* mtx, double frequency = 10.0);
    ~HiloIntArranque();
};
