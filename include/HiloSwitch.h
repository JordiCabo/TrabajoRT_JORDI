/**
 * @file HiloSwitch.h
 * @brief Threading para multiplexado dinámico de señales de referencia
 * @author Jordi + GitHub Copilot
 * @date 2026-01-03
 * 
 * Implementa un hilo POSIX que ejecuta un SignalSwitch para permitir
 * cambio dinámico de la señal de referencia (escalón, rampa, senoidal, PWM)
 * sin interrumpir la ejecución del lazo de control.
 */

#ifndef HILO_SWITCH_H
#define HILO_SWITCH_H

#include <pthread.h>
#include "SignalSwitch.h"
#include "ParametrosCompartidos.h"

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
     * @brief Constructor que crea e inicia el hilo de generación de señal
     * 
     * @param signalSwitch Puntero al multiplexor de señales configurado
     * @param output Puntero a variable de salida compartida donde escribir la señal generada
     * @param running Puntero a variable booleana de control
     * @param mtx Puntero al mutex POSIX compartido
     * @param params Puntero a ParametrosCompartidos para leer signal_type dinámicamente
     * @param frequency Frecuencia de ejecución en Hz (debe ser >= todas las señales)
     * 
     * @note El hilo comienza a ejecutarse inmediatamente
     * @note La frecuencia debe ser lo suficientemente alta para muestrear la señal más rápida
     * @note El SignalSwitch debe estar configurado con las señales a usar
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

    SignalGenerator::SignalSwitch* signalSwitch_;  ///< Puntero al multiplexor
    double* output_;                                ///< Variable de salida
    bool* running_;                                 ///< Flag de ejecución
    pthread_mutex_t* mtx_;                          ///< Mutex POSIX compartido
    ParametrosCompartidos* params_;                 ///< Parámetros compartidos (signal_type)
    double frequency_;                              ///< Frecuencia de ejecución (Hz)
    pthread_t thread_;                              ///< ID del hilo pthread
};

#endif // HILO_SWITCH_H
