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
#include <memory>
#include <atomic>
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
     * @brief Constructor con smart pointers (recomendado)
     */
    HiloPID(std::shared_ptr<DiscreteSystem> pid, 
            std::shared_ptr<VariablesCompartidas> vars,
            std::shared_ptr<ParametrosCompartidos> params, 
            double frequency=100);

    /**
     * @brief Constructor con punteros crudos (compatibilidad)
     * @deprecated Usar constructor con smart pointers
     */
    HiloPID(DiscreteSystem* pid, VariablesCompartidas* vars, 
            ParametrosCompartidos* params, double frequency=100);

    pthread_t getThread() const { return thread_; }

    ~HiloPID();

private:
    // Smart pointers
    std::shared_ptr<DiscreteSystem> system_;
    std::shared_ptr<VariablesCompartidas> vars_;
    std::shared_ptr<ParametrosCompartidos> params_;
    
    // Raw pointers (compatibilidad)
    DiscreteSystem* system_raw_;
    VariablesCompartidas* vars_raw_;
    ParametrosCompartidos* params_raw_;

    double frequency_;
    pthread_t thread_;

    static void* threadFunc(void* arg);
    void run();
};

} // namespace DiscreteSystems
