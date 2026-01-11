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
#include <memory>
#include <atomic>
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
     * @brief Constructor con smart pointers (recomendado)
     * 
     * @param system Smart pointer al sistema discreto a ejecutar
     * @param input Smart pointer a variable de entrada
     * @param output Smart pointer a variable de salida
     * @param running Smart pointer a variable booleana de control
     * @param mtx Smart pointer al mutex que protege variables compartidas
     * @param frequency Frecuencia de ejecución en Hz
     * 
     * @note Esta es la interfaz recomendada para nuevo código
     */
    Hilo(std::shared_ptr<DiscreteSystem> system, 
         std::shared_ptr<double> input, 
         std::shared_ptr<double> output, 
         std::shared_ptr<std::atomic<bool>> running,
         std::shared_ptr<pthread_mutex_t> mtx, 
         double frequency=100);

    /**
     * @brief Constructor con punteros crudos (compatibilidad)
     * @deprecated Usar constructor con smart pointers en nuevo código
     * 
     * @param system Puntero al sistema discreto a ejecutar
     * @param input Puntero a variable de entrada
     * @param output Puntero a variable de salida
     * @param running Puntero a variable booleana de control
     * @param mtx Puntero al mutex que protege variables compartidas
     * @param frequency Frecuencia de ejecución en Hz
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
     */
    ~Hilo();

private:
    // Miembros smart pointer (nueva interfaz)
    std::shared_ptr<DiscreteSystem> system_;
    std::shared_ptr<double> input_;
    std::shared_ptr<double> output_;
    std::shared_ptr<std::atomic<bool>> running_;
    std::shared_ptr<pthread_mutex_t> mtx_;
    
    // Miembros puntero crudo (interfaz antigua, compatibilidad)
    DiscreteSystem* system_raw_;
    double* input_raw_;
    double* output_raw_;
    bool* running_raw_;
    pthread_mutex_t* mtx_raw_;

    pthread_t thread_;
    double frequency_;

    static void* threadFunc(void* arg);
    void run();
};

} // namespace DiscreteSystems
