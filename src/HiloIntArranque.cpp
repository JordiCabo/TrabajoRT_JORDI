#include "HiloIntArranque.h"
#include "../include/Temporizador.h"
#include <iostream>
#include <stdexcept>
#include <csignal>

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
                                 std::shared_ptr<bool> running, 
                                 std::shared_ptr<pthread_mutex_t> mtx, 
                                 double frequency)
    : interruptor_(interruptor), running_(running), mtx_(mtx), frequency_(frequency)
{
    g_running_ptr = running_.get();
    instalar_manejador_signal();
    int ret = pthread_create(&thread_, nullptr, &HiloIntArranque::threadFunc, this);
    if (ret != 0) {
        std::cerr << "ERROR HiloIntArranque: pthread_create failed with code " << ret << std::endl;
        throw std::runtime_error("HiloIntArranque: Failed to create thread");
    }
}

HiloIntArranque::~HiloIntArranque() {
    int ret = pthread_join(thread_, nullptr);
    if (ret != 0) {
        std::cerr << "WARNING HiloIntArranque: pthread_join failed with code " << ret << std::endl;
    }
}

void* HiloIntArranque::threadFunc(void* arg) {
    HiloIntArranque* self = static_cast<HiloIntArranque*>(arg);
    self->run();
    return nullptr;
}

void HiloIntArranque::run() {
    DiscreteSystems::Temporizador timer(frequency_);
    
    while (true) {
        if (!g_signal_run) {
            pthread_mutex_lock(mtx_.get());
            *running_ = false;
            pthread_mutex_unlock(mtx_.get());
            break;
        }
        
        int run_state = interruptor_->getRun();
        pthread_mutex_lock(mtx_.get());
        *running_ = (run_state != 0);
        pthread_mutex_unlock(mtx_.get());
        if (run_state == 0) {
            break;
        }
        
        timer.esperar();  // Temporización absoluta
    }
}
