/**
 * @file Hilo.cpp
 * @brief Implementación del wrapper de threading para sistemas discretos
 * @author Jordi + GitHub Copilot
 * @date 2025-12-18
 */

#include "Hilo.h"
#include "../include/Temporizador.h"

namespace DiscreteSystems {

/**
 * @brief Constructor con smart pointers
 */
Hilo::Hilo(std::shared_ptr<DiscreteSystem> system, 
           std::shared_ptr<double> input, 
           std::shared_ptr<double> output, 
           bool* running,
           std::shared_ptr<pthread_mutex_t> mtx, 
           double frequency)
    : system_(system), input_(input), output_(output), running_(running), mtx_(mtx), 
      frequency_(frequency), system_raw_(nullptr), input_raw_(nullptr), output_raw_(nullptr),
      running_raw_(nullptr), mtx_raw_(nullptr), logger_("Hilo", 1000), iterations_(0)
{
    logger_.initializeHilo(frequency);
    int ret = pthread_create(&thread_, nullptr, &Hilo::threadFunc, this);
    if (ret != 0) {
        std::cerr << "[Hilo::Hilo] Error: pthread_create falló con código " << ret << std::endl;
        throw std::runtime_error("Hilo::Hilo - pthread_create falló");
    }
}

/**
 * @brief Constructor con punteros crudos (compatibilidad)
 */
Hilo::Hilo(DiscreteSystem* system, double* input, double* output, bool* running,
           pthread_mutex_t* mtx, double frequency)
    : system_(nullptr), input_(nullptr), output_(nullptr), running_(nullptr), mtx_(nullptr),
      frequency_(frequency), system_raw_(system), input_raw_(input), output_raw_(output),
      running_raw_(running), mtx_raw_(mtx), logger_("Hilo", 1000), iterations_(0)
{
    logger_.initializeHilo(frequency);
    int ret = pthread_create(&thread_, nullptr, &Hilo::threadFunc, this);
    if (ret != 0) {
        std::cerr << "[Hilo::Hilo] Error: pthread_create falló con código " << ret << std::endl;
        throw std::runtime_error("Hilo::Hilo - pthread_create falló");
    }
}

/**
 * @brief Destructor que espera a que termine el hilo
 * 
 * Ejecuta pthread_join() para asegurar que el hilo finaliza
 * correctamente antes de destruir el objeto Hilo.
 */
Hilo::~Hilo() {
    int ret = pthread_join(thread_, nullptr);
    if (ret != 0) {
        std::cerr << "[Hilo::~Hilo] Error: pthread_join falló con código " << ret << std::endl;
    }
}

/**
 * @brief Función estática de punto de entrada del hilo pthread
 * 
 * @param arg Puntero a this (el objeto Hilo)
 * @return nullptr
 * 
 * Esta función es llamada por pthread; castea el argumento a Hilo*
 * y invoca el método privado run().
 */
void* Hilo::threadFunc(void* arg) {
    Hilo* self = static_cast<Hilo*>(arg);
    self->run();
    return nullptr;
}

/**
 * @brief Loop principal de ejecución del hilo a frecuencia fija
 * 
 * Ejecuta el sistema en bucle a la frecuencia especificada mientras
 * la variable *running_ sea true. Sincroniza entrada y salida mediante
 * el mutex para evitar condiciones de carrera.
 * 
 * @invariant Período de ejecución = 1/frequency_ segundos
 * @invariant Acceso a *input_, *output_ y *running_ solo dentro de lock_guard
 */
void Hilo::run() {
    Temporizador timer(frequency_);
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

        if (!isRunning) {
            break;
        }

        double input;
        
        // Obtener entrada
        if (input_) {
            input = *input_;
        } else {
            pthread_mutex_lock(mtx_raw_);
            input = *input_raw_;
            pthread_mutex_unlock(mtx_raw_);
        }

        struct timespec t1;
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double t_ejecucion_us;

        // Computar
        DiscreteSystem* sys = system_ ? system_.get() : system_raw_;
        double y = sys->next(input);

        struct timespec t2;
        clock_gettime(CLOCK_MONOTONIC, &t2);
        t_ejecucion_us = (t2.tv_sec - t1.tv_sec) * 1000000.0 + 
                         (t2.tv_nsec - t1.tv_nsec) / 1000.0;

        // Escribir salida
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


} // namespace DiscreteSystems
