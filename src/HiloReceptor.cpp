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
#include <stdexcept>
#include <csignal>

/**
 * @brief Constructor que crea e inicia el hilo pthread con co-propiedad shared_ptr
 */
HiloReceptor::HiloReceptor(std::shared_ptr<Receptor> receptor, 
                           std::shared_ptr<bool> running,
                           std::shared_ptr<pthread_mutex_t> mtx, 
                           double frequency)
    : receptor_(receptor), running_(running), mtx_(mtx), frequency_(frequency)
{
    int ret = pthread_create(&thread_, nullptr, &HiloReceptor::threadFunc, this);
    if (ret != 0) {
        std::cerr << "ERROR HiloReceptor: pthread_create failed with code " << ret << std::endl;
        throw std::runtime_error("HiloReceptor: Failed to create thread");
    }
}

/**
 * @brief Destructor que espera a que termine el hilo
 */
HiloReceptor::~HiloReceptor() {
    void* retVal;
    int ret = pthread_join(thread_, &retVal);
    if (ret != 0) {
        std::cerr << "WARNING HiloReceptor: pthread_join failed with code " << ret << std::endl;
    }
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
 * Usa .get() para acceder a punteros crudos de shared_ptr para POSIX API.
 * 
 * @version v1.0.4+ - Migrado a shared_ptr con co-propiedad
 */
void HiloReceptor::run() {
    DiscreteSystems::Temporizador timer(frequency_);

    while (true) {
        bool isRunning;
        pthread_mutex_lock(mtx_.get());
        isRunning = *running_;
        pthread_mutex_unlock(mtx_.get());

        if (!isRunning)
            break; // salir si se recibió SIGINT/SIGTERM o running es false

        // Recibir datos (receptor ya maneja el mutex internamente)
        receptor_->recibir(); // No reportar error si no hay mensaje

        // Esperar hasta completar el período (temporización absoluta)
        timer.esperar();
    }

    pthread_exit(nullptr);
}
