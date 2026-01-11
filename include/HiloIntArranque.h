#pragma once

#include <pthread.h>
#include <memory>
#include <atomic>
#include <string>
#include "InterruptorArranque.h"
#include "RuntimeLogger.h"

class HiloIntArranque {
private:
    // Smart pointers
    std::shared_ptr<InterruptorArranque> interruptor_;
    bool* running_;
    std::shared_ptr<pthread_mutex_t> mtx_;
    
    // Raw pointers (compatibilidad)
    InterruptorArranque* interruptor_raw_;
    bool* running_raw_;
    pthread_mutex_t* mtx_raw_;
    
    pthread_t thread_;
    double frequency_;
    DiscreteSystems::RuntimeLogger logger_;
    struct timespec t_prev_iteration_;
    int iterations_;
    
    void run();
    static void* threadFunc(void* arg);

public:
    /**
     * @brief Constructor con smart pointers (recomendado)
     */
    HiloIntArranque(std::shared_ptr<InterruptorArranque> interruptor, 
                    bool* running,
                    std::shared_ptr<pthread_mutex_t> mtx, 
                    double frequency,
                    const std::string& log_prefix);
    
    /**
     * @brief Constructor con punteros crudos (compatibilidad)
     * @deprecated Usar constructor con smart pointers
     */
    HiloIntArranque(InterruptorArranque* interruptor, bool* running, 
                    pthread_mutex_t* mtx, double frequency,
                    const std::string& log_prefix);
    
    ~HiloIntArranque();
};
