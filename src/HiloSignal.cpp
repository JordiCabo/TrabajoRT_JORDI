/**
 * @file HiloSignal.cpp
 * @brief Implementación del wrapper de threading para generadores de señal con temporización absoluta
 * @author Jordi + GitHub Copilot
 * @date 2025-12-18
 */

#include "HiloSignal.h"
#include "../include/Temporizador.h"
#include <csignal>
#include <iostream>
#include <stdexcept>

namespace SignalGenerator {

/**
 * @brief Constructor con smart pointers
 */
HiloSignal::HiloSignal(std::shared_ptr<Signal> signal, 
                       std::shared_ptr<double> output,
                       bool* running,
                       std::shared_ptr<pthread_mutex_t> mtx, 
                       double frequency)
    : signal_(signal), output_(output), running_(running), mtx_(mtx), 
      frequency_(frequency), signal_raw_(nullptr), output_raw_(nullptr),
      running_raw_(nullptr), mtx_raw_(nullptr), logger_("HiloSignal", 1000), iterations_(0)
{
    logger_.initializeHilo(frequency);
    int ret = pthread_create(&thread_, nullptr, &HiloSignal::threadFunc, this);
    if (ret != 0) {
        std::cerr << "[HiloSignal] Error: pthread_create falló con código " << ret << std::endl;
        throw std::runtime_error("HiloSignal - pthread_create falló");
    }
}

/**
 * @brief Constructor con punteros crudos (compatibilidad)
 */
HiloSignal::HiloSignal(Signal* signal, double* output, bool* running,
                       pthread_mutex_t* mtx, double frequency)
    : signal_(nullptr), output_(nullptr), running_(nullptr), mtx_(nullptr),
      frequency_(frequency), signal_raw_(signal), output_raw_(output),
      running_raw_(running), mtx_raw_(mtx), logger_("HiloSignal", 1000), iterations_(0)
{
    logger_.initializeHilo(frequency);
    int ret = pthread_create(&thread_, nullptr, &HiloSignal::threadFunc, this);
    if (ret != 0) {
        std::cerr << "[HiloSignal] Error: pthread_create falló con código " << ret << std::endl;
        throw std::runtime_error("HiloSignal - pthread_create falló");
    }
}

/**
 * @brief Destructor que espera a que termine el hilo
 * 
 * Ejecuta pthread_join() para asegurar que el hilo finaliza
 * correctamente antes de destruir el objeto HiloSignal.
 */
HiloSignal::~HiloSignal() {
    int ret = pthread_join(thread_, nullptr);
    if (ret != 0) {
        std::cerr << "[HiloSignal] Error: pthread_join falló con código " << ret << std::endl;
    }
}

/**
 * @brief Función estática de punto de entrada del hilo pthread
 * 
 * @param arg Puntero a this (el objeto HiloSignal)
 * @return nullptr
 * 
 * Esta función es llamada por pthread; castea el argumento a HiloSignal*
 * e invoca el método privado run().
 */
void* HiloSignal::threadFunc(void* arg) {
    HiloSignal* self = static_cast<HiloSignal*>(arg);
    self->run();
    return nullptr;
}

/**
 * @brief Loop principal de generación de señal a frecuencia fija
 * 
 * Ejecuta el generador de señal en bucle a la frecuencia especificada
 * mientras *running_ sea true. Genera muestras y las almacena sincronizadas
 * en la variable de salida compartida mediante el mutex.
 * Usa Temporizador con temporización absoluta para eliminar drift.
 * 
 * @invariant Período de ejecución = 1/frequency_ segundos
 * @invariant Acceso a *output_ y *running_ solo dentro de lock_guard
 */
void HiloSignal::run() {
    DiscreteSystems::Temporizador timer(frequency_);
    const double periodo_us = 1000000.0 / frequency_;
    clock_gettime(CLOCK_MONOTONIC, &t_prev_iteration_);

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

        if (!isRunning)
            break;

        struct timespec t1;
        clock_gettime(CLOCK_MONOTONIC, &t1);

        // Obtener generador de señal
        Signal* sig = signal_ ? signal_.get() : signal_raw_;
        double y = sig->next();

        struct timespec t2;
        clock_gettime(CLOCK_MONOTONIC, &t2);
        double t_ejecucion_us = (t2.tv_sec - t1.tv_sec) * 1000000.0 + 
                                (t2.tv_nsec - t1.tv_nsec) / 1000.0;

        // Guardar salida
        if (output_) {
            *output_ = y;
        } else {
            pthread_mutex_lock(mtx_raw_);
            *output_raw_ = y;
            pthread_mutex_unlock(mtx_raw_);
        }

        struct timespec t3;
        clock_gettime(CLOCK_MONOTONIC, &t3);
        double t_total_us = (t3.tv_sec - t0.tv_sec) * 1000000.0 + 
                            (t3.tv_nsec - t0.tv_nsec) / 1000.0;

        const char* status;
        if (t_total_us > periodo_us) {
            status = "CRITICAL";
        } else if (t_total_us > 0.9 * periodo_us) {
            status = "WARNING";
        } else {
            status = "OK";
        }

        logger_.writeLine(iterations_, 0, t_ejecucion_us, t_total_us, periodo_us, ts_real_us, status);

        timer.esperar();
    }

    pthread_exit(nullptr);
}

} // namespace SignalGenerator
