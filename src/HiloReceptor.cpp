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
      receptor_raw_(nullptr), running_raw_(nullptr), mtx_raw_(nullptr),
      logger_("HiloReceptor", 1000), iterations_(0)
{
    logger_.initializeHilo(frequency);
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
      receptor_raw_(receptor), running_raw_(running), mtx_raw_(mtx),
      logger_("HiloReceptor", 1000), iterations_(0)
{
    logger_.initializeHilo(frequency);
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
    const double periodo_us = 1000000.0 / frequency_;
    clock_gettime(CLOCK_MONOTONIC, &t_prev_iteration_);

    // Obtener punteros a los objetos
    Receptor* rx = receptor_ ? receptor_.get() : receptor_raw_;
    pthread_mutex_t* mtx = mtx_ ? mtx_.get() : mtx_raw_;
    
    if (!rx || !mtx) {
        return;
    }

    while (true) {
        iterations_++;
        struct timespec t0;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        double ts_real_us = (t0.tv_sec - t_prev_iteration_.tv_sec) * 1000000.0 +
                            (t0.tv_nsec - t_prev_iteration_.tv_nsec) / 1000.0;
        t_prev_iteration_ = t0;

        bool isRunning;
        pthread_mutex_lock(mtx);
        isRunning = running_ ? *running_ : *running_raw_;
        pthread_mutex_unlock(mtx);

        if (!isRunning)
            break; // salir si se recibió SIGINT/SIGTERM o running es false

        struct timespec t1;
        clock_gettime(CLOCK_MONOTONIC, &t1);

        // Recibir datos (receptor ya maneja el mutex internamente)
        rx->recibir(); // No reportar error si no hay mensaje

        struct timespec t2;
        clock_gettime(CLOCK_MONOTONIC, &t2);
        double t_ejecucion_us = (t2.tv_sec - t1.tv_sec) * 1000000.0 + 
                                (t2.tv_nsec - t1.tv_nsec) / 1000.0;

        // Esperar hasta completar el período (temporización absoluta)
        timer.esperar();

        struct timespec t3;
        clock_gettime(CLOCK_MONOTONIC, &t3);
        double t_total_us = (t3.tv_sec - t0.tv_sec) * 1000000.0 + 
                            (t3.tv_nsec - t0.tv_nsec) / 1000.0;

        const char* status;
        if (t_total_us > periodo_us) { status = "CRITICAL"; }
        else if (t_total_us > 0.9 * periodo_us) { status = "WARNING"; }
        else { status = "OK"; }

        logger_.writeLine(iterations_, 0, t_ejecucion_us, t_total_us, periodo_us, ts_real_us, status);
    }

    pthread_exit(nullptr);
}
