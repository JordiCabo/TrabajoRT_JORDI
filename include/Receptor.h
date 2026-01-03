/*
 * @file Receptor.h
 * 
 * @author Jordi
 * @author GitHub Copilot (asistencia)
 * 
 * @brief Receptor de parámetros de control via IPC
 * 
 * Recibe a través de colas POSIX:
 *  - Kp, Ki, Kd (ganancias del PID)
 *  - Setpoint (referencia deseada)
 * 
 * Escribe valores en ParametrosCompartidos con sincronización mutex.
 * 
 * Uso típico:
 *   ParametrosCompartidos params;
 *   Receptor rx(&params);
 *   if (rx.inicializar()) {
 *       rx.recibir();  // Lee IPC y actualiza params->kp, params->ki, params->kd
 *   }
 */

#ifndef RECEPTOR_H
#define RECEPTOR_H

#include "comm.h"
#include "ParametrosCompartidos.h"
#include <memory>

class Receptor {
public:
    /**
     * @brief Constructor
     * @param params Puntero a parámetros compartidos
     */
    Receptor(ParametrosCompartidos* params);
    
    /**
     * @brief Destructor
     */
    ~Receptor();
    
    /**
     * @brief Inicializar comunicación como receptor
     * @return true si inicialización exitosa
     */
    bool inicializar();
    
    /**
     * @brief Recibir parámetros desde IPC y actualizar ParametrosCompartidos
     * 
     * Lee kp, ki, kd, setpoint via IPC y los escribe con protección mutex
     * 
     * @return true si recepción exitosa
     */
    bool recibir();
    
    /**
     * @brief Cerrar comunicación
     */
    void cerrar();
    
    /**
     * @brief Verificar si está inicializado
     * @return true si está listo para recibir
     */
    bool estaInicializado() const { return inicializado_; }

private:
    ParametrosCompartidos* params_;
    std::unique_ptr<MQueueComm> comm_;
    bool inicializado_;
};

#endif // RECEPTOR_H
