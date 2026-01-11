/**
 * @file Hilo2in.h
 * @brief Wrapper de threading para ejecutar sistemas discretos con dos entradas en tiempo real
 * @author Jordi + GitHub Copilot
 * @date 2025-12-18
 * 
 * Proporciona ejecución pthread de un sistema discreto a una frecuencia fija,
 * con soporte para dos entradas independientes y protección de variables mediante mutex.
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
#include "RuntimeLogger.h"

// Variable de control global para manejo de señales
extern volatile sig_atomic_t g_signal_run;

namespace DiscreteSystems {

/**
 * @class Hilo2in
 * @brief Ejecutor en tiempo real de sistemas discretos con dos punteros de entrada
 * 
 * Similar a Hilo, pero permite especificar dos punteros de entrada independientes.
 * Útil para sistemas como Sumador que requieren múltiples entradas.
 * 
 * Patrón de uso:
 * @code{.cpp}
 * std::mutex mtx;
 * double ref_var = 1.0, feedback_var = 0.0, error_var = 0.0;
 * bool running = true;
 * 
 * DiscreteSystems::Sumador sumador(Ts);
 * DiscreteSystems::Hilo2in thread(&sumador, &ref_var, &feedback_var, &error_var, 
 *                                 &running, &mtx, 100); // 100 Hz
 * @endcode
 * 
 * @invariant El hilo solo accede a *input1_, *input2_ y *output_ dentro de secciones protegidas por mtx
 * @invariant frequency_ > 0 (Hz)
 */
class Hilo2in {
public:
    /**
     * @brief Constructor con smart pointers (recomendado)
     * 
     * @param system Smart pointer al sistema discreto a ejecutar
     * @param input1 Smart pointer a la primera variable de entrada
     * @param input2 Smart pointer a la segunda variable de entrada
     * @param output Smart pointer a la variable de salida
     * @param running Smart pointer a variable booleana de control
     * @param mtx Smart pointer al mutex POSIX compartido
     * @param frequency Frecuencia de ejecución en Hz
     */
    Hilo2in(std::shared_ptr<DiscreteSystem> system, 
            std::shared_ptr<double> input1, 
            std::shared_ptr<double> input2, 
            std::shared_ptr<double> output, 
             bool* running,
            std::shared_ptr<pthread_mutex_t> mtx, 
            double frequency=100);

    /**
     * @brief Constructor con punteros crudos (compatibilidad)
     * @deprecated Usar constructor con smart pointers
     * 
     * @param system Puntero al sistema discreto a ejecutar (debe soportar dos entradas)
     * @param input1 Puntero a la primera variable de entrada del sistema
     * @param input2 Puntero a la segunda variable de entrada del sistema
     * @param output Puntero a la variable de salida del sistema
     * @param running Puntero a variable booleana de control; cuando es false, el hilo se detiene
     * @param mtx Puntero al mutex que protege las variables compartidas
     * @param frequency Frecuencia de ejecución en Hz (período = 1/frequency)
     */
    Hilo2in(DiscreteSystem* system, double* input1, double* input2, double* output, 
            bool *running, pthread_mutex_t* mtx, double frequency=100);

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
    ~Hilo2in();

private:
    // Smart pointers
    std::shared_ptr<DiscreteSystem> system_;
    std::shared_ptr<double> input1_;
    std::shared_ptr<double> input2_;
    std::shared_ptr<double> output_;
    bool* running_;
    std::shared_ptr<pthread_mutex_t> mtx_;
    
    // Raw pointers (compatibilidad)
    DiscreteSystem* system_raw_;
    double* input1_raw_;
    double* input2_raw_;
    double* output_raw_;
    bool* running_raw_;
    pthread_mutex_t* mtx_raw_;

    double frequency_;          ///< Frecuencia de ejecución en Hz
    pthread_t thread_;          ///< Identificador del hilo pthread
    RuntimeLogger logger_;      ///< Logger de timing
    struct timespec t_prev_iteration_;  ///< Timestamp anterior para cálculo de Ts_Real
    int iterations_;            ///< Contador de iteraciones

    /**
     * @brief Función estática de punto de entrada del hilo
     * 
     * @param arg Puntero a this (el objeto Hilo2in)
     * @return nullptr
     * 
     * @note Esta es la función que pthread llama; internamente invoca run()
     */
    static void* threadFunc(void* arg);

    /**
     * @brief Loop principal de ejecución del hilo
     * 
     * Ejecuta el sistema en bucle a la frecuencia especificada mientras
     * *running_ sea true, pasando ambas entradas. Sincroniza entrada/salida
     * con el mutex.
     */
    void run();
};

} // namespace DiscreteSystems
