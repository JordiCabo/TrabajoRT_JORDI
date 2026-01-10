/**
 * @file HiloSwitch.h
 * @brief Threading para multiplexado dinámico de señales de referencia con temporización absoluta
 * @author Jordi + GitHub Copilot
 * @date 2026-01-10
 * 
 * Implementa un hilo POSIX que ejecuta un SignalSwitch para permitir
 * cambio dinámico de la señal de referencia (escalón, rampa, senoidal, PWM)
 * sin interrumpir la ejecución del lazo de control. Usa Temporizador
 * para evitar drift acumulativo.
 */

#ifndef HILO_SWITCH_H
#define HILO_SWITCH_H

#include <memory>
#include <csignal>
#include "SignalSwitch.h"
#include "ParametrosCompartidos.h"

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
     * @brief Constructor que crea e inicia el hilo de generación de señal
     * 
     * @param signalSwitch shared_ptr al multiplexor de señales configurado
     * @param output shared_ptr a variable de salida compartida donde escribir la señal generada
     * @param running shared_ptr a variable booleana de control
     * @param mtx shared_ptr al mutex POSIX compartido
     * @param params shared_ptr a ParametrosCompartidos para leer signal_type dinámicamente
     * @param frequency Frecuencia de ejecución en Hz (debe ser >= todas las señales)
     * 
     * @note El hilo comienza a ejecutarse inmediatamente
     * @note La frecuencia debe ser lo suficientemente alta para muestrear la señal más rápida
     * @note El SignalSwitch debe estar configurado con las señales a usar
     * @note shared_ptr incrementa el contador de referencias; hilo mantiene co-propiedad
     */
    HiloSwitch(std::shared_ptr<SignalGenerator::SignalSwitch> signalSwitch, std::shared_ptr<double> output,
               std::shared_ptr<bool> running, std::shared_ptr<pthread_mutex_t> mtx, std::shared_ptr<ParametrosCompartidos> params,
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

    std::shared_ptr<SignalGenerator::SignalSwitch> signalSwitch_;  ///< Co-propiedad del multiplexor
    std::shared_ptr<double> output_;                                ///< Co-propiedad de variable de salida
    std::shared_ptr<bool> running_;                                 ///< Co-propiedad de flag de ejecución
    std::shared_ptr<pthread_mutex_t> mtx_;                          ///< Co-propiedad del mutex POSIX
    std::shared_ptr<ParametrosCompartidos> params_;                 ///< Co-propiedad de parámetros compartidos
    double frequency_;                              ///< Frecuencia de ejecución (Hz)
    pthread_t thread_;                              ///< ID del hilo pthread
};

#endif // HILO_SWITCH_H
