/**
 * @file Hilo2in.cpp
 * @brief Implementación del wrapper de threading con dos entradas usando shared_ptr
 * @author Jordi + GitHub Copilot
 * @date 2026-01-10
 */

#include "Hilo2in.h"
#include "../include/Temporizador.h"
#include <iostream>
#include <stdexcept>
#include <csignal>

namespace DiscreteSystems {

/**
 * @brief Constructor que crea e inicia el hilo pthread con dos entradas
 * 
 * Crea un nuevo hilo pthread que ejecutará la función threadFunc,
 * iniciando la simulación del sistema a la frecuencia especificada.
 * 
 * @note shared_ptr incrementa referencias; el hilo mantiene co-propiedad de todos los recursos
 */
Hilo2in::Hilo2in(std::shared_ptr<DiscreteSystem> system, 
                 std::shared_ptr<double> input1, 
                 std::shared_ptr<double> input2, 
                 std::shared_ptr<double> output,
                 std::shared_ptr<bool> running, 
                 std::shared_ptr<pthread_mutex_t> mtx, 
                 double frequency)
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
 * Decrementa referencias de shared_ptr automáticamente.
 */
Hilo2in::~Hilo2in() {
    int ret = pthread_join(thread_, nullptr);
    if (ret != 0) {
        std::cerr << "WARNING Hilo2in: pthread_join failed with code " << ret << std::endl;
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
 * Accesos a shared_ptr:
 * - mtx_.get(): obtiene puntero POSIX raw del mutex
 * - *input1_, *input2_, *output_, *running_: acceso dereferenciado seguro
 * 
 * @invariant Período de ejecución = 1/frequency_ segundos
 * @invariant Acceso a *input1_, *input2_, *output_ y *running_ solo dentro de lock
 */
void Hilo2in::run() {
    // Temporizador con retardo absoluto para evitar drift
    Temporizador timer(frequency_);

    while (true) {
        bool isRunning;
        pthread_mutex_lock(mtx_.get());
        isRunning = *running_;
        pthread_mutex_unlock(mtx_.get());

        if (!isRunning)
            break; // salir si se recibió SIGINT/SIGTERM o running_ es false

        double in1, in2;
        pthread_mutex_lock(mtx_.get());
        in1 = *input1_;
        in2 = *input2_;
        pthread_mutex_unlock(mtx_.get());

        double y = system_->next(in1, in2);

        pthread_mutex_lock(mtx_.get());
        *output_ = y;
        pthread_mutex_unlock(mtx_.get());

        timer.esperar();
    }

    pthread_exit(nullptr);
}

} // namespace DiscreteSystems
