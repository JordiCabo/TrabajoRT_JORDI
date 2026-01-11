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
 * @brief Constructor
 */
HiloPID::HiloPID(DiscreteSystem* pid, VariablesCompartidas* vars, 
                 ParametrosCompartidos* params, double frequency)
    : system_(pid), vars_(vars), params_(params), frequency_(frequency)
{
    int ret = pthread_create(&thread_, nullptr, &HiloPID::threadFunc, this);
    if (ret != 0) {
        std::cerr << "[HiloPID] Error: pthread_create falló con código " << ret << std::endl;
        throw std::runtime_error("HiloPID - pthread_create falló");
    }
}

/**
 * @brief Destructor que espera a que termine el hilo
 * 
 * Ejecuta pthread_join() para asegurar que el hilo finaliza
 * correctamente antes de destruir el objeto HiloPID.
 */
HiloPID::~HiloPID() {
    int ret = pthread_join(thread_, nullptr);
    if (ret != 0) {
        std::cerr << "[HiloPID] Error: pthread_join falló con código " << ret << std::endl;
    }
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
    
    if (!system_ || !vars_ || !params_) {
        return;
    }
    
    PIDController* pid = dynamic_cast<PIDController*>(system_);
    
    while (true) {
        pthread_mutex_lock(&vars_->mtx);
        bool running = vars_->running;
        pthread_mutex_unlock(&vars_->mtx);
        
        if (!running)
            break;

        // Leer parámetros dinámicos
        pthread_mutex_lock(&params_->mtx);
        double kp = params_->kp;
        double ki = params_->ki;
        double kd = params_->kd;
        pthread_mutex_unlock(&params_->mtx);

        // Actualizar ganancias del PID
        if (pid != nullptr) {
            pid->setGains(kp, ki, kd);
        }

        // Leer entrada, ejecutar PID, escribir salida
        pthread_mutex_lock(&vars_->mtx);
        double input = vars_->e;
        double output = system_->next(input);
        vars_->u = output;
        pthread_mutex_unlock(&vars_->mtx);

        timer.esperar();
    }

    pthread_exit(nullptr);
}

} // namespace DiscreteSystems
