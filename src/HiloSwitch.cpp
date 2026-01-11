/*
 * @file HiloSwitch.cpp
 * 
 * @author Jordi
 * @author GitHub Copilot (asistencia)
 * 
 * @brief Implementación de HiloSwitch con temporización absoluta
 */

#include "HiloSwitch.h"
#include "../include/Temporizador.h"
#include <iostream>
#include <csignal>
#include <stdexcept>

/**
 * @brief Constructor con smart pointers (recomendado)
 */
HiloSwitch::HiloSwitch(std::shared_ptr<SignalGenerator::SignalSwitch> signalSwitch, 
                       std::shared_ptr<double> output,
                       bool* running,
                       std::shared_ptr<pthread_mutex_t> mtx, 
                       std::shared_ptr<ParametrosCompartidos> params,
                       double frequency)
    : signalSwitch_(signalSwitch), output_(output), running_(running), 
      mtx_(mtx), params_(params), frequency_(frequency),
      signalSwitch_raw_(nullptr), output_raw_(nullptr), running_raw_(nullptr),
      mtx_raw_(nullptr), params_raw_(nullptr)
{
    int ret = pthread_create(&thread_, nullptr, &HiloSwitch::threadFunc, this);
    if (ret != 0) {
        std::cerr << "[HiloSwitch] Error: pthread_create falló con código " << ret << std::endl;
        throw std::runtime_error("HiloSwitch - pthread_create falló");
    }
}

/**
 * @brief Constructor con punteros crudos (compatibilidad)
 */
HiloSwitch::HiloSwitch(SignalGenerator::SignalSwitch* signalSwitch, double* output,
                       bool* running, pthread_mutex_t* mtx, ParametrosCompartidos* params,
                       double frequency)
    : signalSwitch_(nullptr), output_(nullptr), running_(nullptr), 
      mtx_(nullptr), params_(nullptr), frequency_(frequency),
      signalSwitch_raw_(signalSwitch), output_raw_(output), running_raw_(running),
      mtx_raw_(mtx), params_raw_(params)
{
    int ret = pthread_create(&thread_, nullptr, &HiloSwitch::threadFunc, this);
    if (ret != 0) {
        std::cerr << "[HiloSwitch] Error: pthread_create falló con código " << ret << std::endl;
        throw std::runtime_error("HiloSwitch - pthread_create falló");
    }
}

/**
 * @brief Destructor que espera a que termine el hilo
 */
HiloSwitch::~HiloSwitch() {
    void* retVal;
    int ret = pthread_join(thread_, &retVal);
    if (ret != 0) {
        std::cerr << "[HiloSwitch] Error: pthread_join falló con código " << ret << std::endl;
    }
}

/**
 * @brief Punto de entrada del hilo pthread
 */
void* HiloSwitch::threadFunc(void* arg) {
    HiloSwitch* self = static_cast<HiloSwitch*>(arg);
    self->run();
    return nullptr;
}

/**
 * @brief Loop principal de ejecución del hilo a frecuencia fija
 * 
 * Lee signal_type de params_ y actualiza el selector antes de ejecutar next().
 * El switch internamente delega al next() de la señal seleccionada.
 * Usa Temporizador con temporización absoluta para eliminar drift.
 */
void HiloSwitch::run() {
    DiscreteSystems::Temporizador timer(frequency_);

    // Obtener punteros a los objetos
    SignalGenerator::SignalSwitch* sig = signalSwitch_ ? signalSwitch_.get() : signalSwitch_raw_;
    double* out = output_ ? output_.get() : output_raw_;
    pthread_mutex_t* mtx = mtx_ ? mtx_.get() : mtx_raw_;
    ParametrosCompartidos* params = params_ ? params_.get() : params_raw_;
    
    if (!sig || !out || !mtx || !params) {
        return;
    }

    while (true) {
        bool isRunning;
        pthread_mutex_lock(mtx);
        isRunning = running_ ? *running_ : *running_raw_;
        pthread_mutex_unlock(mtx);

        if (!isRunning)
            break; // salir si se recibió SIGINT/SIGTERM o running es false

        // Leer signal_type y setpoint de parámetros compartidos
        pthread_mutex_lock(&params->mtx);
        int signal_type = params->signal_type;
        double setpoint = params->setpoint;
        pthread_mutex_unlock(&params->mtx);

        // Actualizar selector del switch según parámetro
        sig->setSelector(signal_type);
        
        // Actualizar offset (setpoint) de la señal seleccionada antes de next()
        // El switch delega, así que actualizamos directamente en las señales
        // Mapeo: 0=step, 1=pwm, 2=sine
        switch (signal_type) {
            case 0:
                sig->getStepSignal()->offset() = setpoint;
                break;
            case 1:
                sig->getPwmSignal()->offset() = setpoint;
                break;
            case 2:
                sig->getSineSignal()->offset() = setpoint;
                break;
        }

        // Ejecutar next() del switch (delega a la señal seleccionada)
        double value = sig->next();

        // Escribir resultado en variable compartida
        pthread_mutex_lock(mtx);
        *out = value;
        pthread_mutex_unlock(mtx);

        // Esperar hasta completar el período (temporización absoluta)
        timer.esperar();
    }

    pthread_exit(nullptr);
}
