/*
 * @file Receptor.cpp
 * 
 * @author Jordi
 * @author GitHub Copilot (asistencia)
 * 
 * @brief Implementación de Receptor de parámetros IPC
 */

#include "Receptor.h"
#include <iostream>

Receptor::Receptor(ParametrosCompartidos* params) 
    : params_(params)
    , comm_(nullptr)
    , inicializado_(false)
{
    if (!params_) {
        std::cerr << "Receptor: Error - puntero a ParametrosCompartidos es nulo" << std::endl;
    }
}

Receptor::~Receptor() {
    cerrar();
}

bool Receptor::inicializar() {
    if (inicializado_) {
        std::cerr << "Receptor: Ya está inicializado" << std::endl;
        return true;
    }
    
    comm_ = std::make_unique<MQueueComm>();
    
    if (!comm_->initParamsQueue(false)) {  // false = receptor
        std::cerr << "Receptor: Error al inicializar cola de parámetros" << std::endl;
        comm_.reset();
        return false;
    }
    
    inicializado_ = true;
    std::cout << "Receptor: Inicializado correctamente" << std::endl;
    return true;
}

bool Receptor::recibir() {
    if (!inicializado_) {
        std::cerr << "Receptor: No inicializado. Llama a inicializar() primero" << std::endl;
        return false;
    }
    
    if (!params_) {
        std::cerr << "Receptor: ParametrosCompartidos es nulo" << std::endl;
        return false;
    }
    
    ParamsMessage msg;
    
    // Recibir mensaje desde IPC
    if (!comm_->receiveParams(msg)) {
        // No es error crítico si no hay mensaje disponible
        return false;
    }
    
    // Escribir parámetros compartidos bajo protección mutex POSIX
    pthread_mutex_lock(&params_->mtx);
    params_->kp = msg.Kp;
    params_->ki = msg.Ki;
    params_->kd = msg.Kd;
    params_->setpoint = msg.setpoint;
    params_->signal_type = msg.signal_type;
    pthread_mutex_unlock(&params_->mtx);
    
    return true;
}

void Receptor::cerrar() {
    if (inicializado_ && comm_) {
        comm_->closeQueues();
        comm_.reset();
        inicializado_ = false;
        std::cout << "Receptor: Cerrado correctamente" << std::endl;
    }
}
