/*
 * @file HiloReceptor.cpp
 * 
 * @author Jordi
 * @author GitHub Copilot (asistencia)
 * 
 * @brief Implementación de HiloReceptor con temporización absoluta
 */

#include "HiloReceptor.h"
#include "../include/Temporizador.h"
#include <iostream>
#include <csignal>

/**
 * @brief Constructor que crea e inicia el hilo pthread
 */
HiloReceptor::HiloReceptor(Receptor* receptor, bool* running,
                           pthread_mutex_t* mtx, double frequency)
    : receptor_(receptor), running_(running), mtx_(mtx), frequency_(frequency)
{
    pthread_create(&thread_, nullptr, &HiloReceptor::threadFunc, this);
}

/**
 * @brief Destructor que espera a que termine el hilo
 */
HiloReceptor::~HiloReceptor() {
    void* retVal;
    pthread_join(thread_, &retVal);
}

/**
 * @brief Punto de entrada del hilo pthread
 */
void* HiloReceptor::threadFunc(void* arg) {
    HiloReceptor* self = static_cast<HiloReceptor*>(arg);
    self->run();
    return nullptr;
}

/**
 * @brief Loop principal de ejecución del hilo a frecuencia fija
 * 
 * Ejecuta receptor->recibir() periódicamente mientras *running_ sea true.
 * Lee running bajo protección mutex para evitar condiciones de carrera.
 * Usa Temporizador con temporización absoluta para eliminar drift.
 */
void HiloReceptor::run() {
    DiscreteSystems::Temporizador timer(frequency_);

    while (true) {
        bool isRunning;
        pthread_mutex_lock(mtx_);
        isRunning = *running_;
        pthread_mutex_unlock(mtx_);

        if (!isRunning)
            break; // salir si se recibió SIGINT/SIGTERM o running es false

        // Recibir datos (receptor ya maneja el mutex internamente)
        receptor_->recibir(); // No reportar error si no hay mensaje

        // Esperar hasta completar el período (temporización absoluta)
        timer.esperar();
    }

    pthread_exit(nullptr);
}
