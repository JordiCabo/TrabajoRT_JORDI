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
           std::shared_ptr<std::atomic<bool>> running,
           std::shared_ptr<pthread_mutex_t> mtx, 
           double frequency)
    : system_(system), input_(input), output_(output), running_(running), mtx_(mtx), 
      frequency_(frequency), system_raw_(nullptr), input_raw_(nullptr), output_raw_(nullptr),
      running_raw_(nullptr), mtx_raw_(nullptr)
{
    pthread_create(&thread_, nullptr, &Hilo::threadFunc, this);
}

/**
 * @brief Constructor con punteros crudos (compatibilidad)
 */
Hilo::Hilo(DiscreteSystem* system, double* input, double* output, bool* running,
           pthread_mutex_t* mtx, double frequency)
    : system_(nullptr), input_(nullptr), output_(nullptr), running_(nullptr), mtx_(nullptr),
      frequency_(frequency), system_raw_(system), input_raw_(input), output_raw_(output),
      running_raw_(running), mtx_raw_(mtx)
{
    pthread_create(&thread_, nullptr, &Hilo::threadFunc, this);
}

/**
 * @brief Destructor que espera a que termine el hilo
 * 
 * Ejecuta pthread_join() para asegurar que el hilo finaliza
 * correctamente antes de destruir el objeto Hilo.
 */
Hilo::~Hilo() {
    pthread_join(thread_, nullptr);
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

    while (true) {
        bool isRunning;
        
        // Detectar interfaz y acceder a running_
        if (running_) {
            // Smart pointer interface
            isRunning = running_->load();
        } else {
            // Raw pointer interface
            pthread_mutex_lock(mtx_raw_);
            isRunning = *running_raw_;
            pthread_mutex_unlock(mtx_raw_);
        }

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

        // Computar
        DiscreteSystem* sys = system_ ? system_.get() : system_raw_;
        double y = sys->next(input);

        // Escribir salida
        if (output_) {
            *output_ = y;
        } else {
            pthread_mutex_lock(mtx_raw_);
            *output_raw_ = y;
            pthread_mutex_unlock(mtx_raw_);
        }

        timer.esperar();
    }

    pthread_exit(nullptr);
}


} // namespace DiscreteSystems
