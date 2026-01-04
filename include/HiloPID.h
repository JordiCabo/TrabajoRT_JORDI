/**
 * @file HiloPID.h
 * @brief Wrapper de threading especializado para controladores PID con parámetros dinámicos
 * @author Jordi + GitHub Copilot
 * @date 2026-01-03
 * 
 * Proporciona ejecución pthread de un PIDController con actualización dinámica de parámetros
 * (Kp, Ki, Kd) mediante ParametrosCompartidos protegidos con mutex.
 */

#pragma once
#include <pthread.h>
#include <iostream>
#include <unistd.h>
#include <mutex>
#include <csignal>
#include "DiscreteSystem.h"
#include "VariablesCompartidas.h"
#include "ParametrosCompartidos.h"

// Variable de control global para manejo de señales
extern volatile sig_atomic_t g_signal_run;

namespace DiscreteSystems {

/**
 * @class HiloPID
 * @brief Ejecutor en tiempo real especializado para PIDController con parámetros dinámicos
 * 
 * Ejecuta un PIDController en un hilo pthread separado a una frecuencia
 * especificada en Hz, leyendo dinámicamente los parámetros Kp, Ki, Kd desde
 * ParametrosCompartidos en cada ciclo. Permite sintonización en línea sin
 * interrumpir la ejecución.
 * 
 * Patrón de uso:
 * @code{.cpp}
 * ParametrosCompartidos params;  // kp, ki, kd, setpoint con mutex
 * VariablesCompartidas vars;     // ref, e, u, yk, ykd, running con mutex
 * 
 * DiscreteSystems::PIDController pid(1.0, 0.5, 0.1, 0.001);
 * DiscreteSystems::HiloPID thread(&pid, &vars, &params, 100); // 100 Hz
 * 
 * // La GUI actualiza params.kp/ki/kd con lock(params.mtx)
 * // El hilo lee y aplica cambios automáticamente cada ciclo
 * // Accede a vars.e (entrada) y vars.u (salida) con lock(vars.mtx)
 * 
 * vars.running = false; // Detiene el hilo
 * @endcode
 * 
 * @invariant El hilo lee parámetros dentro de secciones protegidas por params->mtx
 * @invariant frequency_ > 0 (Hz)
 */
class HiloPID {
public:
    /**
     * @brief Constructor que inicia la ejecución del hilo con parámetros dinámicos
     * 
     * @param pid Puntero al PIDController a ejecutar
     * @param vars Puntero a VariablesCompartidas (ref, e, u, yk, ykd, running con mutex)
     * @param params Puntero a ParametrosCompartidos (kp, ki, kd, setpoint con su propio mutex)
     * @param frequency Frecuencia de ejecución en Hz (período = 1/frequency)
     * 
     * @note El hilo comienza a ejecutarse inmediatamente desde el constructor
     * @note Lee kp/ki/kd de params en cada ciclo, permitiendo sintonización en línea
     * @note Accede a vars->e (entrada) y vars->u (salida) con lock(vars->mtx)
     */
    HiloPID(DiscreteSystem* pid, VariablesCompartidas* vars, 
            ParametrosCompartidos* params, double frequency=100);

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
    ~HiloPID();

private:
    DiscreteSystem* system_;          ///< Puntero al PIDController a ejecutar
    VariablesCompartidas* vars_;      ///< Puntero a variables compartidas (ref, e, u, yk, ykd, running)
    ParametrosCompartidos* params_;   ///< Puntero a parámetros dinámicos (kp, ki, kd, setpoint)
    int iterations_;                  ///< Número de iteraciones ejecutadas
    double frequency_;                ///< Frecuencia de ejecución en Hz

    pthread_t thread_;                ///< Identificador del hilo pthread

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
     * @brief Loop principal de ejecución del hilo con actualización de parámetros
     * 
     * Ejecuta el PID en bucle a la frecuencia especificada mientras
     * vars_->running sea true. En cada ciclo:
     * 1. Lee kp/ki/kd de params_ con lock(params_->mtx)
     * 2. Actualiza el PID con setKp/setKi/setKd
     * 3. Lee vars_->e y escribe vars_->u con lock(vars_->mtx)
     */
    void run();
};

} // namespace DiscreteSystems
