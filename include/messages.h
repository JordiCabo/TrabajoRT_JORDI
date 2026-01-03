/*
 * @file messages.h
 * 
 * @author Corsino (diseño de tipos de mensaje)
 * @author GitHub Copilot (implementación)
 * 
 * @brief Definiciones de estructuras de mensaje para IPC
 * 
 * Define dos tipos de mensaje intercambiados entre procesos:
 *  - DataMessage (57 bytes): datos de control enviados a GUI
 *  - ParamsMessage (37 bytes): parámetros PID recibidos de GUI
 * 
 * Usa serialización manual (sin padding) para portabilidad.
 * 
 * @note Contribuciones:
 *       - Corsino: Diseño de tipos para pedagogía clara
 *       - GitHub Copilot: Implementación de estructuras
 */

#ifndef MESSAGES_H
#define MESSAGES_H

#include <cstdint>

// =====================================================
// ESTRUCTURAS DE MENSAJES
// =====================================================

// Estructura de datos (visualización)
struct DataMessage {
    double values[6];       // Hasta 6 valores (máximo en modo debug)
    double timestamp;       // Marca temporal (segundos)
    uint8_t num_values;     // Número de valores válidos (3 o 6)
};

// Estructura de parámetros (control)
struct ParamsMessage {
    double Kp;
    double Ki;
    double Kd;
    double setpoint;
    uint8_t signal_type;    // 0=Escalón, 1=Rampa, 2=Senoidal
    uint32_t timestamp;     // Marca temporal del envío (milisegundos desde epoch)
};

#endif // MESSAGES_H
