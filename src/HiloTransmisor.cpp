/*
 * @file HiloTransmisor.cpp
 * 
 * @author Jordi
 * @author GitHub Copilot (asistencia)
 * 
 * @brief Implementación de HiloTransmisor con temporización absoluta
 */

#include "HiloTransmisor.h"
#include "../include/Temporizador.h"
#include <iostream>
#include <csignal>

/**
 * @brief Constructor con smart pointers (recomendado)
 */
HiloTransmisor::HiloTransmisor(std::shared_ptr<Transmisor> transmisor, 
                               std::shared_ptr<bool> running,
                               std::shared_ptr<pthread_mutex_t> mtx, 
                               double frequency)
    : transmisor_(transmisor), running_(running), mtx_(mtx), frequency_(frequency),
      transmisor_raw_(nullptr), running_raw_(nullptr), mtx_raw_(nullptr)
{
    pthread_create(&thread_, nullptr, &HiloTransmisor::threadFunc, this);
}

/**
 * @brief Constructor con punteros crudos (compatibilidad)
 */
HiloTransmisor::HiloTransmisor(Transmisor* transmisor, bool* running,
                               pthread_mutex_t* mtx, double frequency)
    : transmisor_(nullptr), running_(nullptr), mtx_(nullptr), frequency_(frequency),
      transmisor_raw_(transmisor), running_raw_(running), mtx_raw_(mtx)
{
    pthread_create(&thread_, nullptr, &HiloTransmisor::threadFunc, this);
}

/**
 * @brief Destructor que espera a que termine el hilo
 */
HiloTransmisor::~HiloTransmisor() {
    void* retVal;
    pthread_join(thread_, &retVal);
}

/**
 * @brief Punto de entrada del hilo pthread
 */
void* HiloTransmisor::threadFunc(void* arg) {
    HiloTransmisor* self = static_cast<HiloTransmisor*>(arg);
    self->run();
    return nullptr;
}

/**
 * @brief Loop principal de ejecución del hilo a frecuencia fija
 * 
 * Ejecuta transmisor->enviar() periódicamente mientras *running_ sea true.
 * Lee running bajo protección mutex para evitar condiciones de carrera.
 * Usa Temporizador con temporización absoluta para eliminar drift.
 */
void HiloTransmisor::run() {
    DiscreteSystems::Temporizador timer(frequency_);

    // Obtener punteros
    Transmisor* trans = transmisor_ ? transmisor_.get() : transmisor_raw_;
    if (!trans) {
        return;
    }

    while (true) {
        bool isRunning;
        
        if (running_) {
            isRunning = *running_;
        } else if (mtx_raw_) {
            pthread_mutex_lock(mtx_raw_);
            isRunning = *running_raw_;
            pthread_mutex_unlock(mtx_raw_);
        } else {
            break;
        }

        if (!isRunning)
            break;

        if (!trans->enviar()) {
            std::cerr << "HiloTransmisor: Error al enviar datos" << std::endl;
        }

        timer.esperar();
    }

    pthread_exit(nullptr);
}
