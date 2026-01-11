/**
 * @file Hilo2in.cpp
 * @brief Implementación del wrapper de threading con dos entradas
 * @author Jordi + GitHub Copilot
 * @date 2025-12-18
 */

#include "Hilo2in.h"
#include "../include/Temporizador.h"
#include <csignal>
#include <iostream>
#include <stdexcept>

namespace DiscreteSystems {

/**
 * @brief Constructor con smart pointers (recomendado)
 */
Hilo2in::Hilo2in(std::shared_ptr<DiscreteSystem> system, 
                 std::shared_ptr<double> input1, 
                 std::shared_ptr<double> input2, 
                 std::shared_ptr<double> output, 
                 bool* running,
                 std::shared_ptr<pthread_mutex_t> mtx, 
                 double frequency)
    : system_(system), input1_(input1), input2_(input2), output_(output),
      running_(running), mtx_(mtx), frequency_(frequency),
      system_raw_(nullptr), input1_raw_(nullptr), input2_raw_(nullptr),
      output_raw_(nullptr), running_raw_(nullptr), mtx_raw_(nullptr),
      logger_("Hilo2in", 1000), iterations_(0)
{
    logger_.initializeHilo(frequency);
    int ret = pthread_create(&thread_, nullptr, &Hilo2in::threadFunc, this);
    if (ret != 0) {
        std::cerr << "[Hilo2in] Error: pthread_create falló con código " << ret << std::endl;
        throw std::runtime_error("Hilo2in - pthread_create falló");
    }
}

/**
 * @brief Constructor con punteros crudos (compatibilidad)
 */
Hilo2in::Hilo2in(DiscreteSystem* system, double* input1, double* input2, double* output,
                 bool *running, pthread_mutex_t* mtx, double frequency)
    : system_(nullptr), input1_(nullptr), input2_(nullptr), output_(nullptr),
      running_(nullptr), mtx_(nullptr), frequency_(frequency),
      system_raw_(system), input1_raw_(input1), input2_raw_(input2),
      output_raw_(output), running_raw_(running), mtx_raw_(mtx),
      logger_("Hilo2in", 1000), iterations_(0)
{
    logger_.initializeHilo(frequency);
    int ret = pthread_create(&thread_, nullptr, &Hilo2in::threadFunc, this);
    if (ret != 0) {
        std::cerr << "[Hilo2in] Error: pthread_create falló con código " << ret << std::endl;
        throw std::runtime_error("Hilo2in - pthread_create falló");
    }
}

/**
 * @brief Destructor que espera a que termine el hilo
 * 
 * Ejecuta pthread_join() para asegurar que el hilo finaliza
 * correctamente antes de destruir el objeto Hilo2in.
 */
Hilo2in::~Hilo2in() {
    int ret = pthread_join(thread_, nullptr);
    if (ret != 0) {
        std::cerr << "[Hilo2in] Error: pthread_join falló con código " << ret << std::endl;
    }
}

/**
 * @brief Función estática de punto de entrada del hilo pthread
 * 
 * @param arg Puntero a this (el objeto Hilo2in)
 * @return nullptr
 * 
 * Esta función es llamada por pthread; castea el argumento a Hilo2in*
 * e invoca el método privado run().
 */
void* Hilo2in::threadFunc(void* arg) {
    Hilo2in* self = static_cast<Hilo2in*>(arg);
    self->run();
    return nullptr;
}

/**
 * @brief Loop principal de ejecución del hilo a frecuencia fija con dos entradas
 * 
 * Ejecuta el sistema en bucle a la frecuencia especificada mientras
 * la variable *running_ sea true. Lee ambas entradas sincronizadas,
 * calcula la salida, y la almacena sincronizada.
 * 
 * @invariant Período de ejecución = 1/frequency_ segundos
 * @invariant Acceso a *input1_, *input2_, *output_ y *running_ solo dentro de lock_guard
 */
void Hilo2in::run() {
    // Temporizador con retardo absoluto para evitar drift
    Temporizador timer(frequency_);

    // Obtener punteros a los objetos
    DiscreteSystem* sys = system_ ? system_.get() : system_raw_;
    double* in1 = input1_ ? input1_.get() : input1_raw_;
    double* in2 = input2_ ? input2_.get() : input2_raw_;
    double* out = output_ ? output_.get() : output_raw_;
    pthread_mutex_t* mtx = mtx_ ? mtx_.get() : mtx_raw_;
    
    if (!sys || !in1 || !in2 || !out || !mtx) {
        return;
    }

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
        pthread_mutex_lock(mtx);
        isRunning = running_ ? *running_ : *running_raw_;
        pthread_mutex_unlock(mtx);

        if (!isRunning)
            break; // salir si se recibió SIGINT/SIGTERM o running_ es false

        double in1_val, in2_val;
        struct timespec t1;
        clock_gettime(CLOCK_MONOTONIC, &t1);
        
        pthread_mutex_lock(mtx);
        in1_val = *in1;
        in2_val = *in2;
        pthread_mutex_unlock(mtx);

        struct timespec t2;
        clock_gettime(CLOCK_MONOTONIC, &t2);
        double t_ejecucion_us;

        double y = sys->next(in1_val, in2_val);

        struct timespec t3;
        clock_gettime(CLOCK_MONOTONIC, &t3);
        t_ejecucion_us = (t3.tv_sec - t2.tv_sec) * 1000000.0 + 
                         (t3.tv_nsec - t2.tv_nsec) / 1000.0;

        pthread_mutex_lock(mtx);
        *out = y;
        pthread_mutex_unlock(mtx);

        struct timespec t4;
        clock_gettime(CLOCK_MONOTONIC, &t4);
        double t_total_us = (t4.tv_sec - t0.tv_sec) * 1000000.0 + 
                            (t4.tv_nsec - t0.tv_nsec) / 1000.0;

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
