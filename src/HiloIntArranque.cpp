#include "HiloIntArranque.h"
#include "../include/Temporizador.h"
#include <csignal>
#include <iostream>
#include <stdexcept>

volatile sig_atomic_t g_signal_run = 1;
bool* g_running_ptr = nullptr;

void manejador_signal(int sig) {
    g_signal_run = 0;
}

void instalar_manejador_signal() {
    signal(SIGINT, manejador_signal);
    signal(SIGTERM, manejador_signal);
}

HiloIntArranque::HiloIntArranque(std::shared_ptr<InterruptorArranque> interruptor, 
                                 bool* running,
                                 std::shared_ptr<pthread_mutex_t> mtx, 
                                 double frequency)
    : interruptor_(interruptor), running_(running), mtx_(mtx), frequency_(frequency),
      interruptor_raw_(nullptr), running_raw_(nullptr), mtx_raw_(nullptr)
{
    instalar_manejador_signal();
    int ret = pthread_create(&thread_, nullptr, &HiloIntArranque::threadFunc, this);
    if (ret != 0) {
        std::cerr << "[HiloIntArranque] Error: pthread_create falló con código " << ret << std::endl;
        throw std::runtime_error("HiloIntArranque - pthread_create falló");
    }
}

HiloIntArranque::HiloIntArranque(InterruptorArranque* interruptor, bool* running, 
                                 pthread_mutex_t* mtx, double frequency)
    : interruptor_(nullptr), running_(nullptr), mtx_(nullptr), frequency_(frequency),
      interruptor_raw_(interruptor), running_raw_(running), mtx_raw_(mtx)
{
    g_running_ptr = running_raw_;
    instalar_manejador_signal();
    int ret = pthread_create(&thread_, nullptr, &HiloIntArranque::threadFunc, this);
    if (ret != 0) {
        std::cerr << "[HiloIntArranque] Error: pthread_create falló con código " << ret << std::endl;
        throw std::runtime_error("HiloIntArranque - pthread_create falló");
    }
}

HiloIntArranque::~HiloIntArranque() {
    int ret = pthread_join(thread_, nullptr);
    if (ret != 0) {
        std::cerr << "[HiloIntArranque] Error: pthread_join falló con código " << ret << std::endl;
    }
}

void* HiloIntArranque::threadFunc(void* arg) {
    HiloIntArranque* self = static_cast<HiloIntArranque*>(arg);
    self->run();
    return nullptr;
}

void HiloIntArranque::run() {
    DiscreteSystems::Temporizador timer(frequency_);
    
    // Obtener punteros
    InterruptorArranque* int_ptr = interruptor_ ? interruptor_.get() : interruptor_raw_;
    if (!int_ptr) {
        return;
    }
    
    while (true) {
        if (!g_signal_run) {
            pthread_mutex_lock(mtx_ ? mtx_.get() : mtx_raw_);
            if (running_) {
                *running_ = false;
            } else {
                *running_raw_ = false;
            }
            pthread_mutex_unlock(mtx_ ? mtx_.get() : mtx_raw_);
            break;
        }
        
        int run_state = int_ptr->getRun();
        
        pthread_mutex_lock(mtx_ ? mtx_.get() : mtx_raw_);
        if (running_) {
            *running_ = (run_state != 0);
        } else {
            *running_raw_ = (run_state != 0);
        }
        pthread_mutex_unlock(mtx_ ? mtx_.get() : mtx_raw_);
        
        if (run_state == 0) {
            break;
        }
        
        timer.esperar();
    }
}
