/*
 * @file Transmisor.h
 * 
 * @author Jordi
 * @author GitHub Copilot (asistencia)
 * 
 * @brief Transmisor de datos de control via IPC
 * 
 * Envía a través de colas POSIX:
 *  - Referencia (ref)
 *  - Acción de control (u)
 *  - Salida de planta (yk)
 * 
 * Lee valores desde VariablesCompartidas con sincronización mutex.
 * 
 * Uso típico:
 *   VariablesCompartidas vars;
 *   Transmisor tx(&vars);
 *   if (tx.inicializar()) {
 *       tx.enviar();  // Lee y envía vars->ref, vars->u, vars->yk
 *   }
 */

#ifndef TRANSMISOR_H
#define TRANSMISOR_H

#include "comm.h"
#include "VariablesCompartidas.h"
#include <memory>

class Transmisor {
public:
    /**
     * @brief Constructor
     * @param vars Puntero a variables compartidas
     */
    Transmisor(VariablesCompartidas* vars);
    
    /**
     * @brief Destructor
     */
    ~Transmisor();
    
    /**
     * @brief Inicializar comunicación como emisor
     * @return true si inicialización exitosa
     */
    bool inicializar();
    
    /**
     * @brief Enviar datos de control desde VariablesCompartidas
     * 
     * Lee ref, u, yk con protección mutex y los envía via IPC
     * 
     * @return true si envío exitoso
     */
    bool enviar();
    
    /**
     * @brief Cerrar comunicación
     */
    void cerrar();
    
    /**
     * @brief Verificar si está inicializado
     * @return true si está listo para enviar
     */
    bool estaInicializado() const { return inicializado_; }
    
    /**
     * @brief Obtener tiempo transcurrido desde inicio
     * @return Tiempo en segundos
     */
    double getTiempoTranscurrido() const;

private:
    VariablesCompartidas* vars_;
    std::unique_ptr<MQueueComm> comm_;
    bool inicializado_;
    double tiempo_inicio_;
};

#endif // TRANSMISOR_H
