/**
 * @file HiloPID.cpp
 * @brief Implementación del wrapper especializado para PID con parámetros dinámicos usando shared_ptr
 * @author Jordi + GitHub Copilot
 * @date 2026-01-10
 */

#include "../include/HiloPID.h"
#include "../include/PIDController.h"
#include "../include/Temporizador.h"
#include <ctime>
#include <csignal>

namespace DiscreteSystems {

/**
 * @brief Constructor que crea e inicia el hilo pthread
 * 
 * Crea un nuevo hilo pthread que ejecutará la función threadFunc,
 * iniciando la simulación del PID con actualización dinámica de parámetros.
 * 
 * @note shared_ptr incrementa referencias; el hilo mantiene co-propiedad de todos los recursos
 */
HiloPID::HiloPID(std::shared_ptr<DiscreteSystem> pid, 
                 std::shared_ptr<VariablesCompartidas> vars, 
                 std::shared_ptr<ParametrosCompartidos> params, 
                 double frequency)
    : system_(pid), vars_(vars), params_(params), frequency_(frequency), iterations_(0) {
    pthread_create(&thread_, nullptr, &HiloPID::threadFunc, this);
}

/**
 * @brief Destructor que espera a que termine el hilo
 * 
 * Ejecuta pthread_join() para asegurar que el hilo finaliza
 * correctamente antes de destruir el objeto HiloPID.
 * Decrementa referencias de shared_ptr automáticamente.
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
 * Accesos a shared_ptr:
 * - vars_.get()->mtx: obtiene puntero raw para pthread_mutex_lock
 * - params_.get()->mtx: obtiene puntero raw para pthread_mutex_lock
 * - *vars_, *params_: acceso seguro garantizado por shared_ptr
 * 
 * @invariant Período de ejecución = 1/frequency_ segundos
 * @invariant Acceso a vars_ y params_ protegido por sus respectivos mutex
 */
void HiloPID::run() {
    // Crear temporizador con retardo absoluto (evita drift acumulativo)
    Temporizador timer(frequency_);

    // Cast a PIDController para usar setGains
    PIDController* pid = dynamic_cast<PIDController*>(system_.get());
    
    while (true) {
        // Verificar si debe seguir ejecutando
        pthread_mutex_lock(&vars_->mtx);
        bool running = vars_->running;
        pthread_mutex_unlock(&vars_->mtx);
        
        if (!running)
            break; // salir si se recibió SIGINT/SIGTERM o running es false

        // 2. Leer parámetros dinámicos (kp, ki, kd) con su propio mutex
        pthread_mutex_lock(&params_->mtx);
        double kp = params_->kp;
        double ki = params_->ki;
        double kd = params_->kd;
        pthread_mutex_unlock(&params_->mtx);

        // 3. Actualizar ganancias del PID (fuera de sección crítica)
        if (pid != nullptr) {
            pid->setGains(kp, ki, kd);
        }

        // 4. Leer entrada (error), ejecutar PID, escribir salida (control)
        pthread_mutex_lock(&vars_->mtx);
        double input = vars_->e;              // Leer error
        double output = system_->next(input); // Ejecutar PID
        vars_->u = output;                    // Escribir acción de control
        pthread_mutex_unlock(&vars_->mtx);

        // 5. Dormir hasta el siguiente período absoluto (sin drift)
        timer.esperar();
        iterations_++;
    }

    pthread_exit(nullptr);
}


} // namespace DiscreteSystems
