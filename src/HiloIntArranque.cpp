#include "HiloIntArranque.h"
#include "../include/Temporizador.h"
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

HiloIntArranque::HiloIntArranque(InterruptorArranque* interruptor, bool* running, pthread_mutex_t* mtx)
    : interruptor_(interruptor), running_(running), mtx_(mtx)
{
    g_running_ptr = running_;
    instalar_manejador_signal();
    pthread_create(&thread_, nullptr, &HiloIntArranque::threadFunc, this);
}

HiloIntArranque::~HiloIntArranque() {
    pthread_join(thread_, nullptr);
}

void* HiloIntArranque::threadFunc(void* arg) {
    HiloIntArranque* self = static_cast<HiloIntArranque*>(arg);
    self->run();
    return nullptr;
}

void HiloIntArranque::run() {
    DiscreteSystems::Temporizador timer(10.0);  // Chequear cada 100ms (10 Hz)
    
    while (true) {
        if (!g_signal_run) {
            pthread_mutex_lock(mtx_);
            *running_ = false;
            pthread_mutex_unlock(mtx_);
            break;
        }
        
        int run_state = interruptor_->getRun();
        pthread_mutex_lock(mtx_);
        *running_ = (run_state != 0);
        pthread_mutex_unlock(mtx_);
        if (run_state == 0) {
            break;
        }
        
        timer.esperar();  // Temporización absoluta
    }
}
