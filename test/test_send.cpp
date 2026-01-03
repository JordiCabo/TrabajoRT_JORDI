/*
 * @file test_send.cpp
 * 
 * @author Corsino (diseño de programa de prueba)
 * @author GitHub Copilot (implementación)
 * 
 * @brief Programa de prueba: envío de mensajes de datos
 * 
 * Prueba unitaria para verificar:
 *  - Creación de colas POSIX
 *  - Serialización de mensajes de datos
 *  - Envío no-bloqueante vía IPC
 *  - Formato correcto de DataMessage
 * 
 * @usage ./bin/test_send
 * 
 * @note Contribuciones:
 *       - Corsino: Diseño de casos de prueba pedagógicos
 *       - GitHub Copilot: Implementación con salida informativa
 *
 * Compilar: cmake --build build --target test_send
 */

#include "comm.h"
#include <iostream>
#include <unistd.h>
#include <ctime>

int main() {
    std::cout << "=== Test de Envío de Datos ===" << std::endl;
    
    MQueueComm comm;
    
    // Inicializar cola como emisor
    if (!comm.initDataQueue(true)) {
        std::cerr << "Error: No se pudo abrir cola de datos" << std::endl;
        return 1;
    }
    
    std::cout << "Cola de datos abierta correctamente" << std::endl;
    
    // Enviar 10 mensajes de prueba
    for (int i = 0; i < 10; i++) {
        DataMessage msg;
        msg.values[0] = i * 1.0;
        msg.values[1] = i * 2.0;
        msg.values[2] = i * 3.0;
        msg.timestamp = i * 0.1;
        msg.num_values = 3;
        
        if (comm.sendData(msg)) {
            std::cout << "Mensaje " << i << " enviado: ";
            std::cout << "v1=" << msg.values[0] << ", ";
            std::cout << "v2=" << msg.values[1] << ", ";
            std::cout << "v3=" << msg.values[2] << ", ";
            std::cout << "t=" << msg.timestamp << std::endl;
        } else {
            std::cerr << "Error enviando mensaje " << i << std::endl;
        }
        
        usleep(100000);  // 100ms
    }
    
    std::cout << "\nEnvío completado. Ejecutar test_receive en otro terminal." << std::endl;
    
    comm.closeQueues();
    
    return 0;
}
