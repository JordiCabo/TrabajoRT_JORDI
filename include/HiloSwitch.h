/*
 * @file HiloSwitch.h
 * 
 * @author Jordi
 * @author GitHub Copilot (asistencia)
 * 
 * @brief Wrapper de threading para SignalSwitch con frecuencia dinámica
 * 
 * Ejecuta el método next() de un SignalSwitch en un hilo pthread
 * a frecuencia fija, permitiendo generación periódica de la señal
 * seleccionada sin bloquear el hilo principal.
 * 
 * Uso típico:
 *   VariablesCompartidas vars;
 *   SignalSwitch sw(step, sine, pwm, 1);
 *   HiloSwitch hiloSw(&sw, &vars.ref, &vars.running, &vars.mtx, 100.0); // 100 Hz
 *   // El hilo genera valores a 100 Hz
 *   sw.setSelector(2); // Cambiar a seno dinámicamente
 *   vars.running = false; // Detener
 */

#ifndef HILO_SWITCH_H
#define HILO_SWITCH_H

#include <pthread.h>
#include "SignalSwitch.h"
#include "ParametrosCompartidos.h"

/**
 * @class HiloSwitch
 * @brief Ejecuta SignalSwitch periódicamente en hilo separado
 * 
 * Envuelve un objeto SignalSwitch y ejecuta su método next() en un
 * hilo pthread a frecuencia configurable. Sincroniza escritura en
 * variable compartida mediante mutex.
 */
class HiloSwitch {
public:
    /**
     * @brief Constructor que crea e inicia el hilo
     * @param signalSwitch Puntero al multiplexor de señales
     * @param output Puntero a variable de salida compartida, se puede usar para meter ruido al sistema
     * @param running Puntero al flag de ejecución (bajo mutex)
     * @param mtx Puntero al mutex compartido POSIX
     * @param params Puntero a parámetros compartidos (para leer signal_type)
     * @param frequency Frecuencia de ejecución en Hz (debe ser >= frecuencia de todas las señales)
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
