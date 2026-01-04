/**
 * @file Hilo.h
 * @brief Wrapper de threading para ejecutar sistemas discretos en tiempo real
 * @author Jordi + GitHub Copilot
 * @date 2025-12-18
 * 
 * Proporciona ejecución pthread de un sistema discreto a una frecuencia fija,
 * con protección de variables compartidas mediante mutex.
 */

#pragma once
#include <pthread.h>
#include <iostream>
#include <unistd.h>
#include <mutex>
#include <csignal>
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
 * @brief Ejecutor en tiempo real de sistemas discretos con un puntero de entrada/salida
 * 
 * Ejecuta un DiscreteSystem en un hilo pthread separado a una frecuencia
 * especificada en Hz. Sincroniza el acceso a variables compartidas mediante
 * un mutex para evitar condiciones de carrera.
 * 
 * Patrón de uso:
 * @code{.cpp}
 * std::mutex mtx;
 * double input_var = 0.0, output_var = 0.0;
 * bool running = true;
 * 
 * DiscreteSystems::PIDController pid(Kp, Ki, Kd, Ts);
 * DiscreteSystems::Hilo thread(&pid, &input_var, &output_var, &running, &mtx, 100); // 100 Hz
 * 
 * // El hilo ejecuta automáticamente; cambiar input_var a través del mutex
 * // la salida aparece en output_var
 * 
 * running = false; // Detiene el hilo
 * // Destructor espera a que termine el hilo
 * @endcode
 * 
 * @invariant El hilo solo accede a *input_ y *output_ dentro de secciones protegidas por mtx
 * @invariant frequency_ > 0 (Hz)
 */
class Hilo {
public:
    /**
     * @brief Constructor que inicia la ejecución del hilo e instala el manejador de señales
     * 
     * @param system Puntero al sistema discreto a ejecutar (PID, función de transferencia, etc.)
     * @param input Puntero a la variable de entrada del sistema
     * @param output Puntero a la variable de salida del sistema
     * @param running Puntero a variable booleana de control; cuando es false, el hilo se detiene
     * @param mtx Puntero al mutex que protege las variables compartidas
     * @param frequency Frecuencia de ejecución en Hz (período = 1/frequency)
     * 
     * @note El hilo comienza a ejecutarse inmediatamente desde el constructor
     * @note El período de muestreo interno del sistema debe coincidir con 1/frequency
     */
    Hilo(DiscreteSystem* system, double* input, double* output, bool *running, 
         pthread_mutex_t* mtx, double frequency=100);

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
     */
    ~Hilo();

private:
    DiscreteSystem* system_;    ///< Puntero al sistema a ejecutar
    double* input_;             ///< Puntero a variable de entrada compartida
    double* output_;            ///< Puntero a variable de salida compartida
    pthread_mutex_t* mtx_;           ///< Puntero al mutex POSIX para sincronización
    int iterations_;            ///< Número de iteraciones a ejecutar
    double frequency_;          ///< Frecuencia de ejecución en Hz
    bool* running_;             ///< Puntero a variable de control de ejecución

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
