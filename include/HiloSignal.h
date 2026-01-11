/**
 * @file HiloSignal.h
 * @brief Wrapper de threading para ejecutar generadores de señal en tiempo real con temporización absoluta
 * @author Jordi + GitHub Copilot
 * @date 2025-12-18
 * 
 * Proporciona ejecución pthread de un generador de señal a una frecuencia fija,
 * con protección de variables compartidas mediante mutex y Temporizador
 * para evitar drift acumulativo.
 */

#pragma once
#include <pthread.h>
#include <mutex>
#include <memory>
#include <atomic>
#include <string>
#include <unistd.h>
#include <csignal>
#include "SignalGenerator.h"
#include "RuntimeLogger.h"

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
     * @brief Constructor con smart pointers (recomendado)
     * 
     * @param signal Smart pointer al generador de señal
     * @param output Smart pointer a variable de salida
     * @param running Smart pointer a variable booleana de control
     * @param mtx Smart pointer al mutex que protege variables compartidas
     * @param frequency Frecuencia de ejecución en Hz
     */
    HiloSignal(std::shared_ptr<Signal> signal, 
               std::shared_ptr<double> output, 
               bool* running,
               std::shared_ptr<pthread_mutex_t> mtx, 
               double frequency,
               const std::string& log_prefix);

    /**
     * @brief Constructor con punteros crudos (compatibilidad)
     * @deprecated Usar constructor con smart pointers en nuevo código
     * 
     * @param signal Puntero al generador de señal
     * @param output Puntero a variable de salida
     * @param running Puntero a variable booleana de control
     * @param mtx Puntero al mutex que protege variables compartidas
     * @param frequency Frecuencia de ejecución en Hz
     */
    HiloSignal(Signal* signal, double* output, bool* running,
               pthread_mutex_t* mtx, double frequency,
               const std::string& log_prefix);

    /**
     * @brief Destructor que espera a que termine el hilo
     */
    ~HiloSignal();

    /**
     * @brief Obtiene el identificador del hilo pthread
     * @return pthread_t ID del hilo
     */
    pthread_t getThread() const { return thread_; }

private:
    // Smart pointers (nueva interfaz)
    std::shared_ptr<Signal> signal_;
    std::shared_ptr<double> output_;
    bool* running_;
    std::shared_ptr<pthread_mutex_t> mtx_;
    
    // Punteros crudos (interfaz antigua, compatibilidad)
    Signal* signal_raw_;
    double* output_raw_;
    bool* running_raw_;
    pthread_mutex_t* mtx_raw_;

    double frequency_;
    pthread_t thread_;
    DiscreteSystems::RuntimeLogger logger_;
    struct timespec t_prev_iteration_;
    int iterations_;

    static void* threadFunc(void* arg);
    void run();
};

} // namespace SignalGenerator
