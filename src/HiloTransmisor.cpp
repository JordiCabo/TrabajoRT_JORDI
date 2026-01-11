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
#include <stdexcept>
#include <ctime>

/**
 * @brief Constructor con smart pointers (recomendado)
 */
HiloTransmisor::HiloTransmisor(std::shared_ptr<Transmisor> transmisor, 
                               bool* running,
                               std::shared_ptr<pthread_mutex_t> mtx, 
                               double frequency)
    : transmisor_(transmisor), running_(running), mtx_(mtx), frequency_(frequency),
      transmisor_raw_(nullptr), running_raw_(nullptr), mtx_raw_(nullptr),
      logger_("HiloTransmisor", 1000), iterations_(0)
{
    logger_.initializeHilo(frequency);
    int ret = pthread_create(&thread_, nullptr, &HiloTransmisor::threadFunc, this);
    if (ret != 0) {
        std::cerr << "[HiloTransmisor] Error: pthread_create falló con código " << ret << std::endl;
        throw std::runtime_error("HiloTransmisor - pthread_create falló");
    }
}

/**
 * @brief Constructor con punteros crudos (compatibilidad)
 */
HiloTransmisor::HiloTransmisor(Transmisor* transmisor, bool* running,
                               pthread_mutex_t* mtx, double frequency)
    : transmisor_(nullptr), running_(nullptr), mtx_(nullptr), frequency_(frequency),
      transmisor_raw_(transmisor), running_raw_(running), mtx_raw_(mtx),
      logger_("HiloTransmisor", 1000), iterations_(0)
{
    logger_.initializeHilo(frequency);
    int ret = pthread_create(&thread_, nullptr, &HiloTransmisor::threadFunc, this);
    if (ret != 0) {
        std::cerr << "[HiloTransmisor] Error: pthread_create falló con código " << ret << std::endl;
        throw std::runtime_error("HiloTransmisor - pthread_create falló");
    }
}

/**
 * @brief Destructor que espera a que termine el hilo
 */
HiloTransmisor::~HiloTransmisor() {
    void* retVal;
    int ret = pthread_join(thread_, &retVal);
    if (ret != 0) {
        std::cerr << "[HiloTransmisor] Error: pthread_join falló con código " << ret << std::endl;
    }
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
    const double periodo_us = 1000000.0 / frequency_;
    clock_gettime(CLOCK_MONOTONIC, &t_prev_iteration_);

    // Obtener punteros
    Transmisor* trans = transmisor_ ? transmisor_.get() : transmisor_raw_;
    if (!trans) {
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
        pthread_mutex_lock(mtx_ ? mtx_.get() : mtx_raw_);
        isRunning = running_ ? *running_ : *running_raw_;
        pthread_mutex_unlock(mtx_ ? mtx_.get() : mtx_raw_);

        if (!isRunning) {
            break;
        }

        struct timespec t1;
        clock_gettime(CLOCK_MONOTONIC, &t1);

        if (!trans->enviar()) {
            std::cerr << "HiloTransmisor: Error al enviar datos" << std::endl;
        }

        struct timespec t2;
        clock_gettime(CLOCK_MONOTONIC, &t2);
        double t_ejecucion_us = (t2.tv_sec - t1.tv_sec) * 1000000.0 + 
                                (t2.tv_nsec - t1.tv_nsec) / 1000.0;

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
