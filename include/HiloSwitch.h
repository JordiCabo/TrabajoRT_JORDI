/**
 * @file HiloSwitch.h
 * @brief Threading para multiplexado dinámico de señales de referencia con temporización absoluta
 * @author Jordi + GitHub Copilot
 * @date 2026-01-03
 * 
 * Implementa un hilo POSIX que ejecuta un SignalSwitch para permitir
 * cambio dinámico de la señal de referencia (escalón, rampa, senoidal, PWM)
 * sin interrumpir la ejecución del lazo de control. Usa Temporizador
 * para evitar drift acumulativo.
 */

#ifndef HILO_SWITCH_H
#define HILO_SWITCH_H

#include <pthread.h>
#include <csignal>
#include <memory>
#include <atomic>
#include "SignalSwitch.h"
#include "ParametrosCompartidos.h"
#include "RuntimeLogger.h"

// Variable de control global para manejo de señales
extern volatile sig_atomic_t g_signal_run;

/**
 * @class HiloSwitch
 * @brief Hilo dedicado para multiplexado dinámico de señales de referencia
 * 
 * Ejecuta un SignalSwitch en un hilo pthread separado a frecuencia fija.
 * Permite que la GUI seleccione dinámicamente entre diferentes tipos de
 * señales de referencia (escalón, rampa, senoidal, PWM) modificando
 * signal_type en ParametrosCompartidos.
 * 
 * Generador de referencia:
 * @verbatim
 *                    ┌──────────────┐
 *                    │  signal_type │ (from ParametrosCompartidos)
 *                    │   (1,2,3,4)  │
 *                    └───────┬──────┘
 *                            │
 *     ┌───────┬───────┬──────v──────┬──────────┐
 *     │       │       │              │          │
 *     v       v       v              v          v
 *   Escalón Rampa  Senoidal         PWM     Noise(?)
 *     │       │       │              │          │
 *     └───────┴───────┴──────┬───────┴──────────┘
 *                            │
 *                       HiloSwitch
 *                            │
 *                       next() → output
 *                            │
 *                    VariablesCompartidas.ref
 * @endverbatim
 * 
 * Patrón de uso (en control_simulator):
 * @code{.cpp}
 * VariablesCompartidas vars;
 * ParametrosCompartidos params;
 * 
 * auto step = std::make_shared<SignalGenerator::StepSignal>(0.001, 1.0);
 * auto sine = std::make_shared<SignalGenerator::SineSignal>(0.001, 1.0, 0.5);
 * auto ramp = std::make_shared<SignalGenerator::RampSignal>(0.001, 1.0, 0.1);
 * 
 * SignalGenerator::SignalSwitch sw(step, sine, ramp, 1);  // Inicia en escalón
 * 
 * HiloSwitch hiloSw(&sw, &vars.ref, &vars.running, &vars.mtx, &params, 100.0);
 * 
 * // La GUI cambia signal_type:
 * {
 *     std::lock_guard<pthread_mutex_t> lock(params.mtx);
 *     params.signal_type = 2;  // Cambiar a rampa
 * }
 * // HiloSwitch detecta el cambio y conmuta automáticamente
 * @endcode
 * 
 * @invariant frequency_ > 0 (Hz)
 * @invariant El hilo solo ejecuta signalSwitch->next() mientras *running_ == true
 * @invariant signal_type en params debe ser válido (1-4) para el SignalSwitch
 */
class HiloSwitch {
public:
    /**
     * @brief Constructor con smart pointers (recomendado)
     */
    HiloSwitch(std::shared_ptr<SignalGenerator::SignalSwitch> signalSwitch, 
               std::shared_ptr<double> output,
               bool* running,
               std::shared_ptr<pthread_mutex_t> mtx, 
               std::shared_ptr<ParametrosCompartidos> params,
               double frequency);
    
    /**
     * @brief Constructor con punteros crudos (compatibilidad)
     * @deprecated Usar constructor con smart pointers
     */
    HiloSwitch(SignalGenerator::SignalSwitch* signalSwitch, double* output,
               bool* running, pthread_mutex_t* mtx, ParametrosCompartidos* params,
               double frequency);
    
    /**
     * @brief Destructor que espera terminación del hilo
     */
    ~HiloSwitch();
    
    /**
     * @brief Obtiene el identificador del hilo pthread
     * @return pthread_t ID del hilo
     */
    pthread_t getThread() const { return thread_; }

private:
    // Smart pointers
    std::shared_ptr<SignalGenerator::SignalSwitch> signalSwitch_;
    std::shared_ptr<double> output_;
    bool* running_;
    std::shared_ptr<pthread_mutex_t> mtx_;
    std::shared_ptr<ParametrosCompartidos> params_;
    
    // Raw pointers (compatibilidad)
    SignalGenerator::SignalSwitch* signalSwitch_raw_;
    double* output_raw_;
    bool* running_raw_;
    pthread_mutex_t* mtx_raw_;
    ParametrosCompartidos* params_raw_;
    
    double frequency_;                              ///< Frecuencia de ejecución (Hz)
    pthread_t thread_;                              ///< ID del hilo pthread
    DiscreteSystems::RuntimeLogger logger_;         ///< Logger de timing
    struct timespec t_prev_iteration_;              ///< Timestamp anterior
    int iterations_;                                ///< Contador de iteraciones

    /**
     * @brief Función estática para pthread_create
     * @param arg Puntero a la instancia HiloSwitch
     * @return nullptr
     */
    static void* threadFunc(void* arg);
    
    /**
     * @brief Loop principal del hilo
     * 
     * En cada ciclo:
     * 1. Lee params_->signal_type con lock
     * 2. Actualiza signalSwitch->setSelector(signal_type)
     * 3. Ejecuta signalSwitch->next()
     * 4. Escribe resultado en *output_ con protección mutex
     */
    void run();
};

#endif // HILO_SWITCH_H
