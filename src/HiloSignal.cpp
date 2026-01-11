/**
 * @file HiloSignal.cpp
 * @brief Implementación del wrapper de threading para generadores de señal con temporización absoluta
 * @author Jordi + GitHub Copilot
 * @date 2025-12-18
 */

#include "HiloSignal.h"
#include "../include/Temporizador.h"
#include <csignal>

namespace SignalGenerator {

/**
 * @brief Constructor con smart pointers
 */
HiloSignal::HiloSignal(std::shared_ptr<Signal> signal, 
                       std::shared_ptr<double> output, 
                       std::shared_ptr<std::atomic<bool>> running,
                       std::shared_ptr<pthread_mutex_t> mtx, 
                       double frequency)
    : signal_(signal), output_(output), running_(running), mtx_(mtx), 
      frequency_(frequency), signal_raw_(nullptr), output_raw_(nullptr),
      running_raw_(nullptr), mtx_raw_(nullptr)
{
    pthread_create(&thread_, nullptr, &HiloSignal::threadFunc, this);
}

/**
 * @brief Constructor con punteros crudos (compatibilidad)
 */
HiloSignal::HiloSignal(Signal* signal, double* output, bool* running,
                       pthread_mutex_t* mtx, double frequency)
    : signal_(nullptr), output_(nullptr), running_(nullptr), mtx_(nullptr),
      frequency_(frequency), signal_raw_(signal), output_raw_(output),
      running_raw_(running), mtx_raw_(mtx)
{
    pthread_create(&thread_, nullptr, &HiloSignal::threadFunc, this);
}

/**
 * @brief Destructor que espera a que termine el hilo
 * 
 * Ejecuta pthread_join() para asegurar que el hilo finaliza
 * correctamente antes de destruir el objeto HiloSignal.
 */
HiloSignal::~HiloSignal() {
    pthread_join(thread_, nullptr);
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

        if (!isRunning)
            break;

        // Obtener generador de señal
        Signal* sig = signal_ ? signal_.get() : signal_raw_;
        double y = sig->next();

        // Guardar salida
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

} // namespace SignalGenerator
