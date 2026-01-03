/**
 * @file Hilo2in.cpp
 * @brief Implementación del wrapper de threading con dos entradas
 * @author Jordi + GitHub Copilot
 * @date 2025-12-18
 */

#include "Hilo2in.h"

namespace DiscreteSystems {

/**
 * @brief Constructor que crea e inicia el hilo pthread con dos entradas
 * 
 * Crea un nuevo hilo pthread que ejecutará la función threadFunc,
 * iniciando la simulación del sistema a la frecuencia especificada.
 */
Hilo2in::Hilo2in(DiscreteSystem* system, double* input1, double* input2, double* output,
                 bool *running, pthread_mutex_t* mtx, double frequency)
    : system_(system), input1_(input1), input2_(input2), output_(output),
      running_(running), mtx_(mtx), frequency_(frequency)
{
    pthread_create(&thread_, nullptr, &Hilo2in::threadFunc, this);
}

/**
 * @brief Destructor que espera a que termine el hilo
 * 
 * Ejecuta pthread_join() para asegurar que el hilo finaliza
 * correctamente antes de destruir el objeto Hilo2in.
 */
Hilo2in::~Hilo2in() {
    pthread_join(thread_, nullptr);
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
    int sleep_us = static_cast<int>(1e6 / frequency_); // microsegundos

    while (true) {
        bool isRunning;
        pthread_mutex_lock(mtx_);
        isRunning = *running_;
        pthread_mutex_unlock(mtx_);

        if (!isRunning)
            break; // salir del bucle si running_ es false

        double in1, in2;
        pthread_mutex_lock(mtx_);
        in1 = *input1_;
        in2 = *input2_;
        pthread_mutex_unlock(mtx_);

        double y = system_->next(in1, in2);

        pthread_mutex_lock(mtx_);
        *output_ = y;
        pthread_mutex_unlock(mtx_);

        usleep(sleep_us);
    }

    int* retVal = new int(0); // valor de retorno opcional
    pthread_exit(retVal);
}

} // namespace DiscreteSystems
