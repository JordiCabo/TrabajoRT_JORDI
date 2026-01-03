/*
 * @file HiloTransmisor.cpp
 * 
 * @author Jordi
 * @author GitHub Copilot (asistencia)
 * 
 * @brief Implementación de HiloTransmisor
 */

#include "HiloTransmisor.h"
#include <unistd.h>
#include <iostream>

/**
 * @brief Constructor que crea e inicia el hilo pthread
 */
HiloTransmisor::HiloTransmisor(Transmisor* transmisor, bool* running,
                               pthread_mutex_t* mtx, double frequency)
    : transmisor_(transmisor), running_(running), mtx_(mtx), frequency_(frequency)
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
 */
void HiloTransmisor::run() {
    int sleep_us = static_cast<int>(1e6 / frequency_); // microsegundos

    while (true) {
        bool isRunning;
        pthread_mutex_lock(mtx_);
        isRunning = *running_;
        pthread_mutex_unlock(mtx_);

        if (!isRunning)
            break; // salir del bucle si running es false

        // Enviar datos (transmisor ya maneja el mutex internamente)
        if (!transmisor_->enviar()) {
            std::cerr << "HiloTransmisor: Error al enviar datos" << std::endl;
        }

        // Esperar hasta completar el período
        usleep(sleep_us);
    }

    int* retVal = new int(0);
    pthread_exit(retVal);
}
