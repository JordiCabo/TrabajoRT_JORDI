/**
 * @file HiloPID.h
 * @brief Wrapper de threading especializado para controladores PID con parámetros dinámicos
 * @author Jordi + GitHub Copilot
 * @date 2026-01-10
 * 
 * Proporciona ejecución pthread de un PIDController con actualización dinámica de parámetros
 * (Kp, Ki, Kd) mediante ParametrosCompartidos protegidos con mutex.
 * 
 * Utiliza shared_ptr para garantizar ciclo de vida seguro de objetos compartidos
 * entre múltiples hilos.
 */

#pragma once
#include <pthread.h>
#include <iostream>
#include <unistd.h>
#include <mutex>
#include <csignal>
#include <memory>
#include "DiscreteSystem.h"
#include "VariablesCompartidas.h"
#include "ParametrosCompartidos.h"

// Variable de control global para manejo de señales
extern volatile sig_atomic_t g_signal_run;

namespace DiscreteSystems {

/**
 * @class HiloPID
 * @brief Ejecutor en tiempo real especializado para PIDController con parámetros dinámicos y ciclo de vida seguro
 * 
 * Ejecuta un PIDController en un hilo pthread separado a una frecuencia
 * especificada en Hz, leyendo dinámicamente los parámetros Kp, Ki, Kd desde
 * ParametrosCompartidos en cada ciclo. Permite sintonización en línea sin
 * interrumpir la ejecución.
 * 
 * Utiliza shared_ptr para variables compartidas, garantizando que:
 * - El PID no se destruye mientras el hilo está activo
 * - Los objetos VariablesCompartidas y ParametrosCompartidos existen durante acceso
 * - No hay fugas de recursos por ciclos de vida mal gestionados
 * 
 * Patrón de uso (v1.0.4+):
 * @code{.cpp}
 * auto params = std::make_shared<ParametrosCompartidos>();  
 * auto vars = std::make_shared<VariablesCompartidas>();     
 * 
 * auto pid = std::make_shared<DiscreteSystems::PIDController>(1.0, 0.5, 0.1, 0.001);
 * DiscreteSystems::HiloPID thread(pid, vars, params, 100); // 100 Hz
 * 
 * // La GUI actualiza params->kp/ki/kd con lock(params->mtx)
 * // El hilo lee y aplica cambios automáticamente cada ciclo
 * // Accede a vars->e (entrada) y vars->u (salida) con lock(vars->mtx)
 * 
 * *vars->running = false; // Detiene el hilo
 * @endcode
 * 
 * @invariant El hilo lee parámetros dentro de secciones protegidas por params->mtx
 * @invariant frequency_ > 0 (Hz)
 * @invariant Propiedad compartida: shared_ptr garantiza validez hasta destrucción del hilo
 */
class HiloPID {
public:
    /**
     * @brief Constructor que inicia la ejecución del hilo con parámetros dinámicos
     * 
     * @param pid shared_ptr al PIDController a ejecutar
     * @param vars shared_ptr a VariablesCompartidas (ref, e, u, yk, ykd, running con mutex)
     * @param params shared_ptr a ParametrosCompartidos (kp, ki, kd, setpoint con su propio mutex)
     * @param frequency Frecuencia de ejecución en Hz (período = 1/frequency)
     * 
     * @note El hilo comienza a ejecutarse inmediatamente desde el constructor
     * @note Lee kp/ki/kd de params en cada ciclo, permitiendo sintonización en línea
     * @note Accede a vars->e (entrada) y vars->u (salida) con lock(vars->mtx)
     * @note shared_ptr incrementa el contador de referencias; hilo mantiene co-propiedad
     */
    HiloPID(std::shared_ptr<DiscreteSystem> pid, 
            std::shared_ptr<VariablesCompartidas> vars, 
            std::shared_ptr<ParametrosCompartidos> params, 
            double frequency=100);

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
     * shared_ptr decrementa referencias automáticamente.
     */
    ~HiloPID();

private:
    std::shared_ptr<DiscreteSystem> system_;          ///< Co-propiedad del PIDController a ejecutar
    std::shared_ptr<VariablesCompartidas> vars_;      ///< Co-propiedad de variables compartidas
    std::shared_ptr<ParametrosCompartidos> params_;   ///< Co-propiedad de parámetros dinámicos
    
    int iterations_;                  ///< Número de iteraciones ejecutadas
    double frequency_;                ///< Frecuencia de ejecución en Hz
    pthread_t thread_;                ///< Identificador del hilo pthread

    /**
     * @brief Función estática de punto de entrada del hilo
     * 
     * @param arg Puntero a this (el objeto HiloPID)
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
