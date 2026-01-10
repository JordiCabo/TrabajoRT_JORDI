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
 * @brief Constructor que crea e inicia el hilo de generación de señal
 * 
 * Crea un nuevo hilo pthread que ejecutará la función threadFunc,
 * generando muestras de la señal a la frecuencia especificada.
 */
HiloSignal::HiloSignal(Signal* signal, double* output, bool* running,
                       pthread_mutex_t* mtx, double frequency)
    : signal_(signal), output_(output),
      running_(running), mtx_(mtx), frequency_(frequency)
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
        pthread_mutex_lock(mtx_);
        isRunning = *running_;
        pthread_mutex_unlock(mtx_);

        if (!isRunning)
            break; // salir si se recibió SIGINT/SIGTERM o running es false

        // Generar siguiente muestra de la señal
        double y = signal_->next();

        // Guardarla en la salida compartida
        pthread_mutex_lock(mtx_);
        *output_ = y;
        pthread_mutex_unlock(mtx_);

        timer.esperar();
    }

    pthread_exit(nullptr);
}

} // namespace SignalGenerator
