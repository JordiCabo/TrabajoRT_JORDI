#pragma once

#include <pthread.h>
#include <memory>
#include <atomic>
#include "InterruptorArranque.h"

class HiloIntArranque {
private:
    // Smart pointers
    std::shared_ptr<InterruptorArranque> interruptor_;
    std::shared_ptr<std::atomic<bool>> running_;
    std::shared_ptr<pthread_mutex_t> mtx_;
    
    // Raw pointers (compatibilidad)
    InterruptorArranque* interruptor_raw_;
    bool* running_raw_;
    pthread_mutex_t* mtx_raw_;
    
    pthread_t thread_;
    double frequency_;
    
    void run();
    static void* threadFunc(void* arg);

public:
    /**
     * @brief Constructor con smart pointers (recomendado)
     */
    HiloIntArranque(std::shared_ptr<InterruptorArranque> interruptor, 
                    std::shared_ptr<std::atomic<bool>> running, 
                    std::shared_ptr<pthread_mutex_t> mtx, 
                    double frequency = 10.0);
    
    /**
     * @brief Constructor con punteros crudos (compatibilidad)
     * @deprecated Usar constructor con smart pointers
     */
    HiloIntArranque(InterruptorArranque* interruptor, bool* running, 
                    pthread_mutex_t* mtx, double frequency = 10.0);
    
    ~HiloIntArranque();
};
