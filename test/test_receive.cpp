/*
 * @file test_receive.cpp
 * 
 * @author Corsino (diseño de programa de prueba)
 * @author GitHub Copilot (implementación)
 * 
 * @brief Programa de prueba: recepción de mensajes de datos
 * 
 * Prueba unitaria para verificar:
 *  - Apertura de colas POSIX
 *  - Recepción bloqueante vía IPC
 *  - Deserialización de mensajes de datos
 *  - Interpretación correcta de DataMessage
 * 
 * @usage ./bin/test_receive
 *        (En otra terminal: ./bin/test_send)
 * 
 * @note Contribuciones:
 *       - Corsino: Diseño de casos de prueba pedagógicos
 *       - GitHub Copilot: Implementación con validación de datos
 *
 * Compilar: cmake --build build --target test_receive
 */

#include "comm.h"
#include <iostream>
#include <unistd.h>

int main() {
    std::cout << "=== Test de Recepción de Datos ===" << std::endl;
    
    MQueueComm comm;
    
    // Inicializar cola como receptor
    if (!comm.initDataQueue(false)) {
        std::cerr << "Error: No se pudo abrir cola de datos" << std::endl;
        return 1;
    }
    
    std::cout << "Cola de datos abierta correctamente" << std::endl;
    std::cout << "Esperando mensajes... (Ctrl+C para salir)" << std::endl;
    
    // Recibir mensajes continuamente
    while (true) {
        DataMessage msg;
        
        if (comm.receiveData(msg)) {
            std::cout << "Mensaje recibido (" << comm.getSequenceCounter() << "): ";
            std::cout << "v1=" << msg.values[0] << ", ";
            std::cout << "v2=" << msg.values[1] << ", ";
            std::cout << "v3=" << msg.values[2] << ", ";
            std::cout << "t=" << msg.timestamp << ", ";
            std::cout << "num=" << (int)msg.num_values << std::endl;
        }
        
        usleep(50000);  // 50ms
    }
    
    comm.closeQueues();
    cleanupQueues();
    
    return 0;
}
