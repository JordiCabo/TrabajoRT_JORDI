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
 * @brief Constructor que crea e inicia el hilo pthread con co-propiedad shared_ptr
 */
HiloTransmisor::HiloTransmisor(std::shared_ptr<Transmisor> transmisor, 
                               std::shared_ptr<bool> running,
                               std::shared_ptr<pthread_mutex_t> mtx, 
                               double frequency)
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
 * Usa Temporizador con temporización absoluta para eliminar drift.
 * Usa .get() para acceder a punteros crudos de shared_ptr para POSIX API.
 * 
 * @version v1.0.4+ - Migrado a shared_ptr con co-propiedad
 */
void HiloTransmisor::run() {
    DiscreteSystems::Temporizador timer(frequency_);

    while (true) {
        bool isRunning;
        pthread_mutex_lock(mtx_.get());
        isRunning = *running_;
        pthread_mutex_unlock(mtx_.get());

        if (!isRunning)
            break; // salir si se recibió SIGINT/SIGTERM o running es false

        // Enviar datos (transmisor ya maneja el mutex internamente)
        if (!transmisor_->enviar()) {
            std::cerr << "HiloTransmisor: Error al enviar datos" << std::endl;
        }

        // Esperar hasta completar el período (temporización absoluta)
        timer.esperar();
    }

    pthread_exit(nullptr);
}
