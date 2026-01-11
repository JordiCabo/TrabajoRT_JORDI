/**
 * @file HiloPID.cpp
 * @brief Implementación del wrapper especializado para PID con parámetros dinámicos
 * @author Jordi + GitHub Copilot
 * @date 2026-01-03
 */

#include "../include/HiloPID.h"
#include "../include/PIDController.h"
#include "../include/Temporizador.h"
#include <ctime>
#include <csignal>

namespace DiscreteSystems {

/**
 * @brief Constructor con smart pointers
 */
HiloPID::HiloPID(std::shared_ptr<DiscreteSystem> pid, 
                 std::shared_ptr<VariablesCompartidas> vars,
                 std::shared_ptr<ParametrosCompartidos> params, 
                 double frequency)
    : system_(pid), vars_(vars), params_(params), frequency_(frequency),
      system_raw_(nullptr), vars_raw_(nullptr), params_raw_(nullptr)
{
    pthread_create(&thread_, nullptr, &HiloPID::threadFunc, this);
}

/**
 * @brief Constructor con punteros crudos (compatibilidad)
 */
HiloPID::HiloPID(DiscreteSystem* pid, VariablesCompartidas* vars, 
                 ParametrosCompartidos* params, double frequency)
    : system_(nullptr), vars_(nullptr), params_(nullptr), frequency_(frequency),
      system_raw_(pid), vars_raw_(vars), params_raw_(params)
{
    pthread_create(&thread_, nullptr, &HiloPID::threadFunc, this);
}

/**
 * @brief Destructor que espera a que termine el hilo
 * 
 * Ejecuta pthread_join() para asegurar que el hilo finaliza
 * correctamente antes de destruir el objeto HiloPID.
 */
HiloPID::~HiloPID() {
    pthread_join(thread_, nullptr);
}

/**
 * @brief Función estática de punto de entrada del hilo pthread
 * 
 * @param arg Puntero a this (el objeto HiloPID)
 * @return nullptr
 * 
 * Esta función es llamada por pthread; castea el argumento a HiloPID*
 * y invoca el método privado run().
 */
void* HiloPID::threadFunc(void* arg) {
    HiloPID* self = static_cast<HiloPID*>(arg);
    self->run();
    return nullptr;
}

/**
 * @brief Loop principal de ejecución del hilo a frecuencia fija
 * 
 * Ejecuta el PID en bucle a la frecuencia especificada mientras
 * vars_->running sea true. En cada ciclo:
 * 1. Verifica estado de ejecución
 * 2. Lee parámetros dinámicos (kp, ki, kd) de params_
 * 3. Actualiza ganancias del PID con setGains()
 * 4. Lee error de entrada, ejecuta PID, escribe acción de control
 * 
 * @invariant Período de ejecución = 1/frequency_ segundos
 * @invariant Acceso a vars_ y params_ protegido por sus respectivos mutex
 */
void HiloPID::run() {
    Temporizador timer(frequency_);

    // Obtener punteros a los objetos
    DiscreteSystem* sys = system_ ? system_.get() : system_raw_;
    VariablesCompartidas* vars = vars_ ? vars_.get() : vars_raw_;
    ParametrosCompartidos* params = params_ ? params_.get() : params_raw_;
    
    if (!sys || !vars || !params) {
        return;
    }
    
    PIDController* pid = dynamic_cast<PIDController*>(sys);
    
    while (true) {
        pthread_mutex_lock(&vars->mtx);
        bool running = vars->running;
        pthread_mutex_unlock(&vars->mtx);
        
        if (!running)
            break;

        // Leer parámetros dinámicos
        pthread_mutex_lock(&params->mtx);
        double kp = params->kp;
        double ki = params->ki;
        double kd = params->kd;
        pthread_mutex_unlock(&params->mtx);

        // Actualizar ganancias del PID
        if (pid != nullptr) {
            pid->setGains(kp, ki, kd);
        }

        // Leer entrada, ejecutar PID, escribir salida
        pthread_mutex_lock(&vars->mtx);
        double input = vars->e;
        double output = sys->next(input);
        vars->u = output;
        pthread_mutex_unlock(&vars->mtx);

        timer.esperar();
    }

    pthread_exit(nullptr);
}

} // namespace DiscreteSystems
