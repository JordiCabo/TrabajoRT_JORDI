/**
 * @file Hilo.h
 * @brief Wrapper de threading para ejecutar sistemas discretos en tiempo real
 * @author Jordi + GitHub Copilot
 * @date 2026-01-10
 * 
 * Proporciona ejecución pthread de un sistema discreto a una frecuencia fija,
 * con protección de variables compartidas mediante mutex.
 * 
 * Utiliza shared_ptr para garantizar ciclo de vida seguro de objetos compartidos
 * entre múltiples hilos.
 */

#pragma once
#include <pthread.h>
#include <iostream>
#include <unistd.h>
#include <mutex>
#include <csignal>
#include <memory>
#include "DiscreteSystem.h"

// Variable de control global para manejo de señales SIGINT/SIGTERM
extern volatile sig_atomic_t g_signal_run;

/**
 * @brief Manejador de señal para SIGINT (Ctrl+C) y SIGTERM (kill)
 * @param sig Número de señal recibida
 */
void manejador_signal(int sig);

/**
 * @brief Instala el manejador de señales para SIGINT y SIGTERM
 */
void instalar_manejador_signal();

namespace DiscreteSystems {

/**
 * @class Hilo
 * @brief Ejecutor en tiempo real de sistemas discretos con cycle management seguro
 * 
 * Ejecuta un DiscreteSystem en un hilo pthread separado a una frecuencia
 * especificada en Hz. Sincroniza el acceso a variables compartidas mediante
 * un mutex para evitar condiciones de carrera.
 * 
 * Utiliza shared_ptr para variables compartidas, garantizando que:
 * - El sistema discreto no se destruye mientras el hilo está activo
 * - Las variables de entrada/salida existen durante el acceso
 * - El mutex es válido mientras hay operaciones en progreso
 * - No hay fugas de recursos por ciclos de vida mal gestionados
 * 
 * Patrón de uso (v1.0.4+):
 * @code{.cpp}
 * auto system = std::make_shared<DiscreteSystems::PIDController>(Kp, Ki, Kd, Ts);
 * auto input = std::make_shared<double>(0.0);
 * auto output = std::make_shared<double>(0.0);
 * auto running = std::make_shared<bool>(true);
 * auto mtx = std::make_shared<pthread_mutex_t>();
 * pthread_mutex_init(mtx.get(), nullptr);
 * 
 * DiscreteSystems::Hilo thread(system, input, output, running, mtx, 100); // 100 Hz
 * 
 * // El hilo ejecuta automáticamente; cambiar *input a través del mutex
 * // la salida aparece en *output
 * 
 * *running = false; // Detiene el hilo
 * // Destructor espera a que termine el hilo
 * @endcode
 * 
 * @invariant El hilo solo accede a *input_ y *output_ dentro de secciones protegidas por mtx_
 * @invariant frequency_ > 0 (Hz)
 * @invariant Propiedad compartida: shared_ptr garantiza validez hasta destrucción del hilo
 */
class Hilo {
public:
    /**
     * @brief Constructor que inicia la ejecución del hilo e instala el manejador de señales
     * 
     * @param system shared_ptr al sistema discreto a ejecutar (PID, función de transferencia, etc.)
     * @param input shared_ptr a la variable de entrada del sistema (doble puntero indirección)
     * @param output shared_ptr a la variable de salida del sistema
     * @param running shared_ptr a variable booleana de control; cuando es false, el hilo se detiene
     * @param mtx shared_ptr al mutex POSIX que protege las variables compartidas
     * @param frequency Frecuencia de ejecución en Hz (período = 1/frequency)
     * 
     * @note El hilo comienza a ejecutarse inmediatamente desde el constructor
     * @note El período de muestreo interno del sistema debe coincidir con 1/frequency
     * @note shared_ptr incrementa el contador de referencias; hilo mantiene co-propiedad
     */
    Hilo(std::shared_ptr<DiscreteSystem> system, 
         std::shared_ptr<double> input, 
         std::shared_ptr<double> output, 
         std::shared_ptr<bool> running,
         std::shared_ptr<pthread_mutex_t> mtx, 
         double frequency=100);

    /**
     * @brief Obtiene el identificador del hilo pthread
     * @return pthread_t ID del hilo
     */
    pthread_t getThread() const { return thread_; }

    /**
     * @brief Destructor que espera a que termine el hilo
     * 
     * Ejecuta pthread_join() para asegurar que el hilo finaliza
     * correctamente antes de destruir el objeto.
     * shared_ptr decremente referencias automáticamente.
     */
    ~Hilo();

private:
    std::shared_ptr<DiscreteSystem> system_;    ///< Co-propiedad del sistema a ejecutar
    std::shared_ptr<double> input_;             ///< Co-propiedad de variable de entrada compartida
    std::shared_ptr<double> output_;            ///< Co-propiedad de variable de salida compartida
    std::shared_ptr<pthread_mutex_t> mtx_;      ///< Co-propiedad del mutex POSIX para sincronización
    std::shared_ptr<bool> running_;             ///< Co-propiedad de variable de control de ejecución
    
    int iterations_;            ///< Número de iteraciones a ejecutar
    double frequency_;          ///< Frecuencia de ejecución en Hz
    pthread_t thread_;          ///< Identificador del hilo pthread

    /**
     * @brief Función estática de punto de entrada del hilo
     * 
     * @param arg Puntero a this (el objeto Hilo)
     * @return nullptr
     * 
     * @note Esta es la función que pthread llama; internamente invoca run()
     */
    static void* threadFunc(void* arg);

    /**
     * @brief Loop principal de ejecución del hilo
     * 
     * Ejecuta el sistema en bucle a la frecuencia especificada mientras
     * *running_ sea true. Sincroniza entrada/salida con el mutex.
     */
    void run();
};

} // namespace DiscreteSystems
