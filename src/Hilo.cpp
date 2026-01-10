/**
 * @file Hilo.cpp
 * @brief Implementación del wrapper de threading para sistemas discretos con shared_ptr
 * @author Jordi + GitHub Copilot
 * @date 2026-01-10
 */

#include "Hilo.h"
#include "../include/Temporizador.h"

namespace DiscreteSystems {

/**
 * @brief Constructor que crea e inicia el hilo pthread e instala el manejador de señales
 * 
 * Crea un nuevo hilo pthread que ejecutará la función threadFunc,
 * iniciando la simulación del sistema a la frecuencia especificada.
 * Instala automáticamente el manejador de señales SIGINT/SIGTERM.
 * 
 * @note shared_ptr incrementa referencias; el hilo mantiene co-propiedad de todos los recursos
 */
Hilo::Hilo(std::shared_ptr<DiscreteSystem> system, 
           std::shared_ptr<double> input, 
           std::shared_ptr<double> output, 
           std::shared_ptr<bool> running,
           std::shared_ptr<pthread_mutex_t> mtx, 
           double frequency)
    : system_(system), input_(input), output_(output), mtx_(mtx), frequency_(frequency), running_(running)  
{
    pthread_create(&thread_, nullptr, &Hilo::threadFunc, this);
}

/**
 * @brief Destructor que espera a que termine el hilo
 * 
 * Ejecuta pthread_join() para asegurar que el hilo finaliza
 * correctamente antes de destruir el objeto Hilo.
 * Decrementa referencias de shared_ptr automáticamente.
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
 * Accesos a shared_ptr:
 * - mtx_.get(): obtiene puntero POSIX raw del mutex
 * - *input_, *output_, *running_: acceso dereferenciado seguro (shared_ptr valida puntero)
 * 
 * @invariant Período de ejecución = 1/frequency_ segundos
 * @invariant Acceso a *input_, *output_ y *running_ solo dentro de lock_guard
 */
void Hilo::run() {
    // Temporizador con retardo absoluto para evitar drift
    Temporizador timer(frequency_);

    while (true) {
        bool isRunning;
        pthread_mutex_lock(mtx_.get());
        isRunning = *running_;
        pthread_mutex_unlock(mtx_.get());

        if (!isRunning) {
            std::cerr << "[Hilo::run] Saliendo: isRunning=" << isRunning << std::endl;
            break; // salir si se recibió SIGINT/SIGTERM o running_ es false
        }

        double input;
        pthread_mutex_lock(mtx_.get());
        input = *input_;
        pthread_mutex_unlock(mtx_.get());

        double y = system_->next(input);

        pthread_mutex_lock(mtx_.get());
        *output_ = y;
        pthread_mutex_unlock(mtx_.get());

        timer.esperar();
    }

    pthread_exit(nullptr);
}


} // namespace DiscreteSystems
