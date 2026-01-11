/**
 * @file Hilo2in.cpp
 * @brief Implementación del wrapper de threading con dos entradas
 * @author Jordi + GitHub Copilot
 * @date 2025-12-18
 */

#include "Hilo2in.h"
#include "../include/Temporizador.h"
#include "system_config.h"
#include <csignal>
#include <iostream>
#include <stdexcept>
#include <ctime>

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
                 double frequency,
                 const std::string& log_prefix)
    : system_(system), input1_(input1), input2_(input2), output_(output),
      running_(running), mtx_(mtx), frequency_(frequency),
      system_raw_(nullptr), input1_raw_(nullptr), input2_raw_(nullptr),
      output_raw_(nullptr), running_raw_(nullptr), mtx_raw_(nullptr),
      logger_(log_prefix, SystemConfig::BUFFER_SIZE_LOGGER),
      t_prev_iteration_(0.0), iterations_(0)
{
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
                 bool *running, pthread_mutex_t* mtx, double frequency,
                 const std::string& log_prefix)
    : system_(nullptr), input1_(nullptr), input2_(nullptr), output_(nullptr),
      running_(nullptr), mtx_(nullptr), frequency_(frequency),
      system_raw_(system), input1_raw_(input1), input2_raw_(input2),
      output_raw_(output), running_raw_(running), mtx_raw_(mtx),
      logger_(log_prefix, SystemConfig::BUFFER_SIZE_LOGGER),
      t_prev_iteration_(0.0), iterations_(0)
{
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
    
    // Inicializar logger
    logger_.initializeHilo(frequency_);

    // Obtener punteros a los objetos
    DiscreteSystem* sys = system_ ? system_.get() : system_raw_;
    double* in1 = input1_ ? input1_.get() : input1_raw_;
    double* in2 = input2_ ? input2_.get() : input2_raw_;
    double* out = output_ ? output_.get() : output_raw_;
    pthread_mutex_t* mtx = mtx_ ? mtx_.get() : mtx_raw_;
    
    if (!sys || !in1 || !in2 || !out || !mtx) {
        return;
    }
    
    struct timespec t_start, t_end;
    const double period_us = 1e6 / frequency_;

    while (true) {
        clock_gettime(CLOCK_MONOTONIC, &t_start);
        
        bool isRunning;
        pthread_mutex_lock(mtx);
        isRunning = running_ ? *running_ : *running_raw_;
        pthread_mutex_unlock(mtx);

        if (!isRunning)
            break; // salir si se recibió SIGINT/SIGTERM o running_ es false

        // Medir t_wait (tiempo esperando en lock)
        struct timespec t_before_read, t_after_read;
        clock_gettime(CLOCK_MONOTONIC, &t_before_read);
        
        double in1_val, in2_val;
        
        pthread_mutex_lock(mtx);
        in1_val = *in1;
        in2_val = *in2;
        pthread_mutex_unlock(mtx);
        
        clock_gettime(CLOCK_MONOTONIC, &t_after_read);

        double y = sys->next(in1_val, in2_val);

        pthread_mutex_lock(mtx);
        *out = y;
        pthread_mutex_unlock(mtx);
        
        clock_gettime(CLOCK_MONOTONIC, &t_end);
        
        // Calcular tiempos
        double t_wait_us = (t_after_read.tv_sec - t_before_read.tv_sec) * 1e6
                         + (t_after_read.tv_nsec - t_before_read.tv_nsec) / 1000.0;
        double t_total_us = (t_end.tv_sec - t_start.tv_sec) * 1e6
                          + (t_end.tv_nsec - t_start.tv_nsec) / 1000.0;
        
        // Calcular período real (ts_real)
        double ts_real_us = 0.0;
        if (iterations_ > 0) {
            double t_current = t_end.tv_sec * 1e6 + t_end.tv_nsec / 1000.0;
            ts_real_us = t_current - t_prev_iteration_;
        }
        t_prev_iteration_ = t_end.tv_sec * 1e6 + t_end.tv_nsec / 1000.0;
        
        // t_ejec = t_total - t_wait
        double t_ejec_us = t_total_us - t_wait_us;
        
        // Determinar status
        double perc_computation = (t_total_us / period_us) * 100.0;
        const char* status = "OK";
        if (perc_computation > (SystemConfig::CRITICAL_THRESHOLD * 100.0)) {
            status = "CRITICAL";
        } else if (perc_computation > (SystemConfig::WARNING_THRESHOLD * 100.0)) {
            status = "WARNING";
        }
        
        // Guardar en logger
        logger_.writeLine(iterations_, t_wait_us, t_ejec_us, t_total_us, 
                         period_us, ts_real_us, status);
        
        iterations_++;
        
        if (iterations_ % SystemConfig::LOGGER_FLUSH_INTERVAL == 0) {
            logger_.flush();
        }

        timer.esperar();
    }

    pthread_exit(nullptr);
}

} // namespace DiscreteSystems
