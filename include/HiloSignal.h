/**
 * @file HiloSignal.h
 * @brief Wrapper de threading para ejecutar generadores de señal en tiempo real
 * @author Jordi + GitHub Copilot
 * @date 2025-12-18
 * 
 * Proporciona ejecución pthread de un generador de señal a una frecuencia fija,
 * con protección de variables compartidas mediante mutex.
 */

#pragma once
#include <pthread.h>
#include <mutex>
#include <unistd.h>
#include <csignal>
#include "SignalGenerator.h"

// Variable de control global para manejo de señales
extern volatile sig_atomic_t g_signal_run;

namespace SignalGenerator {

/**
 * @class HiloSignal
 * @brief Ejecutor en tiempo real de generadores de señal
 * 
 * Ejecuta un generador de señal (Signal) en un hilo pthread separado a una
 * frecuencia especificada en Hz. Típicamente usado para generar señales de
 * prueba (step, sine, ramp) que alimentan sistemas bajo control.
 * 
 * Patrón de uso:
 * @code{.cpp}
 * std::mutex mtx;
 * double signal_output = 0.0;
 * bool running = true;
 * 
 * auto sine_signal = std::make_shared<SignalGenerator::SineSignal>(0.001, 1.0, 2.0);
 * SignalGenerator::HiloSignal signal_thread(sine_signal.get(), &signal_output, &running, &mtx, 100);
 * 
 * // El hilo genera la señal automáticamente; la salida aparece en signal_output
 * 
 * running = false; // Detiene el hilo
 * // Destructor espera a que termine el hilo
 * @endcode
 * 
 * @invariant El hilo solo accede a *output_ dentro de secciones protegidas por mtx
 * @invariant frequency_ > 0 (Hz)
 */
class HiloSignal {
public:
    /**
     * @brief Constructor que inicia la ejecución del hilo generador
     * 
     * @param signal Puntero al generador de señal (Step, Sine, PWM, etc.)
     * @param output Puntero a la variable donde almacenar la salida generada
     * @param running Puntero a variable booleana de control; cuando es false, el hilo se detiene
     * @param mtx Puntero al mutex que protege las variables compartidas
     * @param frequency Frecuencia de ejecución en Hz (período = 1/frequency)
     * 
     * @note El hilo comienza a ejecutarse inmediatamente desde el constructor
     * @note El período de muestreo de la señal debe coincidir con 1/frequency
     */
    HiloSignal(Signal* signal, double* output, bool* running,
               pthread_mutex_t* mtx, double frequency);

    /**
     * @brief Destructor que espera a que termine el hilo
     * 
     * Ejecuta pthread_join() para asegurar que el hilo finaliza
     * correctamente antes de destruir el objeto.
     */
    ~HiloSignal();

    /**
     * @brief Obtiene el identificador del hilo pthread
     * @return pthread_t ID del hilo
     */
    pthread_t getThread() const { return thread_; }

private:
    Signal* signal_;            ///< Puntero al generador de señal
    double* output_;            ///< Puntero a variable de salida compartida
    bool* running_;             ///< Puntero a variable de control de ejecución
    pthread_mutex_t* mtx_;           ///< Puntero al mutex POSIX para sincronización
    double frequency_;          ///< Frecuencia de ejecución en Hz

    pthread_t thread_;          ///< Identificador del hilo pthread

    /**
     * @brief Función estática de punto de entrada del hilo
     * 
     * @param arg Puntero a this (el objeto HiloSignal)
     * @return nullptr
     * 
     * @note Esta es la función que pthread llama; internamente invoca run()
     */
    static void* threadFunc(void* arg);

    /**
     * @brief Loop principal de ejecución del hilo
     * 
     * Ejecuta el generador de señal en bucle a la frecuencia especificada
     * mientras *running_ sea true. Sincroniza la salida con el mutex.
     */
    void run();
};

} // namespace SignalGenerator
