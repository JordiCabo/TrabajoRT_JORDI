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

/**
 * @brief Constructor que crea e inicia el hilo pthread
 */
HiloSwitch::HiloSwitch(std::shared_ptr<SignalGenerator::SignalSwitch> signalSwitch, std::shared_ptr<double> output,
                       std::shared_ptr<bool> running, std::shared_ptr<pthread_mutex_t> mtx, std::shared_ptr<ParametrosCompartidos> params,
                       double frequency)
    : signalSwitch_(signalSwitch), output_(output), running_(running), 
      mtx_(mtx), params_(params), frequency_(frequency)
{
    pthread_create(&thread_, nullptr, &HiloSwitch::threadFunc, this);
}

/**
 * @brief Destructor que espera a que termine el hilo
 */
HiloSwitch::~HiloSwitch() {
    void* retVal;
    pthread_join(thread_, &retVal);
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

    while (true) {
        bool isRunning;
        pthread_mutex_lock(mtx_.get());
        isRunning = *running_;
        pthread_mutex_unlock(mtx_.get());

        if (!isRunning)
            break; // salir si se recibió SIGINT/SIGTERM o running es false

        // Leer signal_type y setpoint de parámetros compartidos
        pthread_mutex_lock(&params_->mtx);
        int signal_type = params_->signal_type;
        double setpoint = params_->setpoint;
        pthread_mutex_unlock(&params_->mtx);

        // Actualizar selector del switch según parámetro
        signalSwitch_->setSelector(signal_type);
        
        // Actualizar offset (setpoint) de la señal seleccionada antes de next()
        // El switch delega, así que actualizamos directamente en las señales
        // Mapeo: 0=step, 1=pwm, 2=sine
        switch (signal_type) {
            case 0:
                signalSwitch_->getStepSignal()->offset() = setpoint;
                break;
            case 1:
                signalSwitch_->getPwmSignal()->offset() = setpoint;
                break;
            case 2:
                signalSwitch_->getSineSignal()->offset() = setpoint;
                break;
        }

        // Ejecutar next() del switch (delega a la señal seleccionada)
        double value = signalSwitch_->next();

        // Escribir resultado en variable compartida
        pthread_mutex_lock(mtx_.get());
        *output_ = value;
        pthread_mutex_unlock(mtx_.get());

        // Esperar hasta completar el período (temporización absoluta)
        timer.esperar();
    }

    pthread_exit(nullptr);
}
