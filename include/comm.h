/*
 * @file comm.h
 * 
 * @author Corsino (diseño de interfaz IPC)
 * @author GitHub Copilot (implementación de serialización)
 * 
 * @brief Interfaz de comunicación IPC con POSIX Message Queues
 * 
 * Define la API pública para:
 *  - Serialización/deserialización de mensajes (sin padding)
 *  - Gestión de colas POSIX (crear, cerrar, enviar, recibir)
 *  - Tipos de datos para intercambio entre procesos
 * 
 * @note Contribuciones:
 *       - Corsino: Decisión de usar serialización manual para evitar padding
 *         y hacer código portable entre arquitecturas
 *       - GitHub Copilot: Implementación de funciones de serialización
 *         con soporte para endianness
 */

#ifndef COMM_H
#define COMM_H

#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <cstdint>
#include <cstring>
#include "messages.h"
#include "comm_config.h"

// =====================================================
// FUNCIONES DE SERIALIZACIÓN
// =====================================================

// Serializar DataMessage a buffer de bytes
size_t serializeDataMessage(const DataMessage& msg, uint8_t* buffer);

// Deserializar buffer de bytes a DataMessage
size_t deserializeDataMessage(const uint8_t* buffer, DataMessage& msg);

// Serializar ParamsMessage a buffer de bytes
size_t serializeParamsMessage(const ParamsMessage& msg, uint8_t* buffer);

// Deserializar buffer de bytes a ParamsMessage
size_t deserializeParamsMessage(const uint8_t* buffer, ParamsMessage& msg);

// =====================================================
// CLASE DE COMUNICACIÓN
// =====================================================

class MQueueComm {
public:
    // Constructor/Destructor
    MQueueComm();
    ~MQueueComm();
    
    // Inicialización
    bool initDataQueue(bool as_sender);      // true=sender (proceso), false=receiver (GUI)
    bool initParamsQueue(bool as_sender);    // true=sender (GUI), false=receiver (proceso)
    
    // Envío (serializa internamente)
    bool sendData(const DataMessage& msg);
    bool sendParams(const ParamsMessage& msg);
    
    // Recepción (deserializa internamente)
    bool receiveData(DataMessage& msg);
    bool receiveParams(ParamsMessage& msg);
    
    // Limpieza
    void closeQueues();
    
    // Obtener contador de secuencia
    uint32_t getSequenceCounter() const { return sequence_counter_; }
    
private:
    mqd_t mq_data_;
    mqd_t mq_params_;
    uint32_t sequence_counter_;  // Para generar sequence en GUI
};

// Función auxiliar para eliminar colas (llamar al finalizar procesos)
void cleanupQueues();

#endif // COMM_H
