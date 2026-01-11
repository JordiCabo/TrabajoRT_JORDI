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
#include <stdexcept>
#include <ctime>

/**
 * @brief Constructor con smart pointers (recomendado)
 */
HiloReceptor::HiloReceptor(std::shared_ptr<Receptor> receptor, 
                           bool* running,
                           std::shared_ptr<pthread_mutex_t> mtx, 
                           double frequency)
    : receptor_(receptor), running_(running), mtx_(mtx), frequency_(frequency),
    receptor_raw_(nullptr), running_raw_(nullptr), mtx_raw_(nullptr)
{
    int ret = pthread_create(&thread_, nullptr, &HiloReceptor::threadFunc, this);
    if (ret != 0) {
        std::cerr << "[HiloReceptor] Error: pthread_create falló con código " << ret << std::endl;
        throw std::runtime_error("HiloReceptor - pthread_create falló");
    }
}

/**
 * @brief Constructor con punteros crudos (compatibilidad)
 */
HiloReceptor::HiloReceptor(Receptor* receptor, bool* running,
                           pthread_mutex_t* mtx, double frequency)
    : receptor_(nullptr), running_(nullptr), mtx_(nullptr), frequency_(frequency),
    receptor_raw_(receptor), running_raw_(running), mtx_raw_(mtx)
{
    int ret = pthread_create(&thread_, nullptr, &HiloReceptor::threadFunc, this);
    if (ret != 0) {
        std::cerr << "[HiloReceptor] Error: pthread_create falló con código " << ret << std::endl;
        throw std::runtime_error("HiloReceptor - pthread_create falló");
    }
}

/**
 * @brief Destructor que espera a que termine el hilo
 */
HiloReceptor::~HiloReceptor() {
    void* retVal;
    int ret = pthread_join(thread_, &retVal);
    if (ret != 0) {
        std::cerr << "[HiloReceptor] Error: pthread_join falló con código " << ret << std::endl;
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
 */
void HiloReceptor::run() {
    DiscreteSystems::Temporizador timer(frequency_);

    // Obtener punteros a los objetos
    Receptor* rx = receptor_ ? receptor_.get() : receptor_raw_;
    pthread_mutex_t* mtx = mtx_ ? mtx_.get() : mtx_raw_;
    
    if (!rx || !mtx) {
        return;
    }

    while (true) {
        bool isRunning;
        pthread_mutex_lock(mtx);
        isRunning = running_ ? *running_ : *running_raw_;
        pthread_mutex_unlock(mtx);

        if (!isRunning)
            break; // salir si se recibió SIGINT/SIGTERM o running es false

        // Recibir datos (receptor ya maneja el mutex internamente)
        rx->recibir(); // No reportar error si no hay mensaje

        timer.esperar();
    }

    pthread_exit(nullptr);
}
