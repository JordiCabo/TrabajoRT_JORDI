/**
 * @file Transmisor.h
 * @brief Transmisor de datos de control hacia GUI via IPC
 * @author Jordi + GitHub Copilot
 * @date 2026-01-03
 * 
 * Envía datos del lazo de control (referencia, acciones de control, realimentación)
 * a través de colas de mensajes POSIX (mqueue) hacia la aplicación Qt para
 * visualización en tiempo real.
 */

#ifndef TRANSMISOR_H
#define TRANSMISOR_H

#include "comm.h"
#include "VariablesCompartidas.h"
#include <memory>

/**
 * @class Transmisor
 * @brief Envío de datos del lazo de control hacia proceso GUI
 * 
 * Parte del sistema IPC que permite visualizar en tiempo real
 * el comportamiento del lazo de control en la GUI Qt (gui_app).
 * 
 * Funciona como una máquina de estados:
 * 1. Inicializar(): conecta a mqueue, preparado para enviar
 * 2. Enviar(): lee datos de VariablesCompartidas y envía a mqueue
 * 3. Cerrar(): desconecta de mqueue
 * 
 * Patrón de uso (en control_simulator):
 * @code{.cpp}
 * VariablesCompartidas vars;
 * Transmisor transmisor(&vars);
 * 
 * if (transmisor.inicializar()) {
 *     while (sistema_ejecutándose) {
 *         // El lazo de control actualiza vars.ref, vars.u, vars.yk con lock(vars.mtx)
 *         transmisor.enviar();  // Envía datos a GUI para visualización
 *     }
 *     transmisor.cerrar();
 * }
 * @endcode
 * 
 * @invariant Una vez inicializado, solo puede estar en estado envío
 * @invariant Acceso a vars->* requiere lock(vars->mtx) en enviar()
 */
class Transmisor {
public:
    /**
     * @brief Constructor
     * @param vars Puntero a estructura VariablesCompartidas que contiene datos del lazo
     */
    Transmisor(VariablesCompartidas* vars);
    
    /**
     * @brief Destructor. Llama a cerrar() si está inicializado.
     */
    ~Transmisor();
    
    /**
     * @brief Inicializar la comunicación como transmisor de datos
     * 
     * Conecta a la mqueue de datos (MQueueComm::DATA_QUEUE_NAME)
     * para enviar muestras a la GUI.
     * 
     * @return true si la inicialización fue exitosa, false si hay error
     * @note Debe llamarse antes de cualquier enviar()
     */
    bool inicializar();
    
    /**
     * @brief Enviar muestras del lazo de control a la GUI
     * 
     * Lee datos de VariablesCompartidas con protección mutex:
     * - ref (referencia)
     * - u (salida del PID)
     * - yk (salida de la planta)
     * 
     * Construye un DataMessage con estos valores y lo envía de forma
     * no-bloqueante a través de mqueue. Incluye marca temporal.
     * 
     * @return true si envío exitoso, false si error de comunicación
     * @note Esta función es no-bloqueante (puede descartarse si cola llena)
     * @warning Debe llamarse solo después de inicializar()
     */
    bool enviar();
    
    /**
     * @brief Cerrar comunicación y liberar recursos
     * 
     * Desconecta de la mqueue y marca el transmisor como no inicializado.
     */
    void cerrar();
    
    /**
     * @brief Verificar si el transmisor está inicializado y listo
     * @return true si está inicializado y puede enviar datos
     */
    bool estaInicializado() const { return inicializado_; }
    
    /**
     * @brief Obtener tiempo transcurrido desde inicio de ejecución
     * @return Tiempo en segundos desde la inicialización
     */
    double getTiempoTranscurrido() const;

private:
    VariablesCompartidas* vars_;                 ///< Puntero a estructura de variables a leer
    std::unique_ptr<MQueueComm> comm_;           ///< Comunicación IPC (mqueue)
    bool inicializado_;                          ///< Flag de inicialización
    double tiempo_inicio_;                       ///< Marca temporal de inicio para calcular tiempo transcurrido
};

#endif // TRANSMISOR_H
