/**
 * @file HiloPID.h
 * @brief Wrapper de threading especializado para controladores PID con parámetros dinámicos
 * @author Jordi + GitHub Copilot
 * @date 2026-01-11
 * @version 1.0.6 - Added timing instrumentation and logging
 * 
 * Proporciona ejecución pthread de un PIDController con actualización dinámica de parámetros
 * (Kp, Ki, Kd) mediante ParametrosCompartidos protegidos con mutex.
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
#include "VariablesCompartidas.h"
#include "ParametrosCompartidos.h"
#include "RuntimeLogger.h"

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
     * @brief Constructor
     */
        HiloPID(DiscreteSystem* pid, VariablesCompartidas* vars, 
            ParametrosCompartidos* params, double frequency,
            const std::string& log_prefix);

    pthread_t getThread() const { return thread_; }
    int getIterations() const { return iterations_; }  // Obtener número de iteración actual

    ~HiloPID();

private:
    DiscreteSystem* system_;
    VariablesCompartidas* vars_;
    ParametrosCompartidos* params_;

    double frequency_;
    pthread_t thread_;
    int iterations_;           // Contador de iteraciones
    struct timespec t_prev_iteration_;  // Timestamp de la iteración anterior
    RuntimeLogger logger_;      // Sistema de logging con buffer circular

    static void* threadFunc(void* arg);
    void run();
};

} // namespace DiscreteSystems
