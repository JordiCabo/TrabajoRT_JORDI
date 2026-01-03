/*
 * @file Transmisor.cpp
 * 
 * @author Jordi
 * @author GitHub Copilot (asistencia)
 * 
 * @brief Implementación de Transmisor de datos IPC
 */

#include "Transmisor.h"
#include <iostream>
#include <ctime>

Transmisor::Transmisor(VariablesCompartidas* vars) 
    : vars_(vars)
    , comm_(nullptr)
    , inicializado_(false)
    , tiempo_inicio_(0.0)
{
    if (!vars_) {
        std::cerr << "Transmisor: Error - puntero a VariablesCompartidas es nulo" << std::endl;
    }
}

Transmisor::~Transmisor() {
    cerrar();
}

bool Transmisor::inicializar() {
    if (inicializado_) {
        std::cerr << "Transmisor: Ya está inicializado" << std::endl;
        return true;
    }
    
    comm_ = std::make_unique<MQueueComm>();
    
    if (!comm_->initDataQueue(true)) {  // true = emisor
        std::cerr << "Transmisor: Error al inicializar cola de datos" << std::endl;
        comm_.reset();
        return false;
    }
    
    // Registrar tiempo de inicio
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    tiempo_inicio_ = ts.tv_sec + ts.tv_nsec / 1e9;
    
    inicializado_ = true;
    std::cout << "Transmisor: Inicializado correctamente" << std::endl;
    return true;
}

bool Transmisor::enviar() {
    if (!inicializado_) {
        std::cerr << "Transmisor: No inicializado. Llama a inicializar() primero" << std::endl;
        return false;
    }
    
    if (!vars_) {
        std::cerr << "Transmisor: VariablesCompartidas es nulo" << std::endl;
        return false;
    }
    
    DataMessage msg;
    
    // Leer variables compartidas bajo protección mutex POSIX
    pthread_mutex_lock(&vars_->mtx);
    msg.values[0] = vars_->ref;
    msg.values[1] = vars_->u;
    msg.values[2] = vars_->yk;
    pthread_mutex_unlock(&vars_->mtx);
    
    msg.num_values = 3;
    
    // Timestamp: tiempo transcurrido desde inicio
    msg.timestamp = getTiempoTranscurrido();
    
    // Enviar mensaje
    if (!comm_->sendData(msg)) {
        std::cerr << "Transmisor: Error al enviar datos" << std::endl;
        return false;
    }
    
    return true;
}

void Transmisor::cerrar() {
    if (inicializado_ && comm_) {
        comm_->closeQueues();
        comm_.reset();
        inicializado_ = false;
        std::cout << "Transmisor: Cerrado correctamente" << std::endl;
    }
}

double Transmisor::getTiempoTranscurrido() const {
    if (!inicializado_) {
        return 0.0;
    }
    
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    double tiempo_actual = ts.tv_sec + ts.tv_nsec / 1e9;
    
    return tiempo_actual - tiempo_inicio_;
}
