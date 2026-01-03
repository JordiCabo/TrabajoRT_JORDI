/**
 * @file Receptor.h
 * @brief Receptor de parámetros PID desde GUI via IPC
 * @author Jordi + GitHub Copilot
 * @date 2026-01-03
 * 
 * Recibe parámetros de control (Kp, Ki, Kd, setpoint) a través de
 * colas de mensajes POSIX (mqueue) y los almacena en ParametrosCompartidos
 * con protección mutex para que el lazo de control los pueda usar de forma segura.
 */

#ifndef RECEPTOR_H
#define RECEPTOR_H

#include "comm.h"
#include "ParametrosCompartidos.h"
#include <memory>

/**
 * @class Receptor
 * @brief Recepción de parámetros PID desde proceso externo (GUI)
 * 
 * Parte del sistema IPC que permite que la aplicación Qt (gui_app)
 * envíe actualizaciones de parámetros al simulador (control_simulator).
 * 
 * Funciona como una máquina de estados:
 * 1. Inicializar(): conecta a mqueue, preparado para recibir
 * 2. Recibir(): lee mensaje de mqueue y actualiza ParametrosCompartidos
 * 3. Cerrar(): desconecta de mqueue
 * 
 * Patrón de uso (en control_simulator):
 * @code{.cpp}
 * ParametrosCompartidos params;
 * Receptor receptor(&params);
 * 
 * if (receptor.inicializar()) {
 *     while (sistema_ejecutándose) {
 *         receptor.recibir();  // Lee parámetros de GUI y actualiza params
 *         // El lazo de control accede a params.kp, .ki, .kd con lock(params.mtx)
 *     }
 *     receptor.cerrar();
 * }
 * @endcode
 * 
 * @invariant Una vez inicializado, solo puede estar en estado recepción
 * @invariant Acceso a params->* requiere lock(params->mtx) en el llamador
 */
class Receptor {
public:
    /**
     * @brief Constructor
     * @param params Puntero a estructura ParametrosCompartidos que almacena Kp, Ki, Kd, setpoint
     */
    Receptor(ParametrosCompartidos* params);
    
    /**
     * @brief Destructor. Llama a cerrar() si está inicializado.
     */
    ~Receptor();
    
    /**
     * @brief Inicializar la comunicación como receptor de parámetros
     * 
     * Conecta a la mqueue de parámetros (MQueueComm::PARAMS_QUEUE_NAME)
     * para escuchar mensajes de la GUI.
     * 
     * @return true si la inicialización fue exitosa, false si hay error
     * @note Debe llamarse antes de cualquier recibir()
     */
    bool inicializar();
    
    /**
     * @brief Recibir un mensaje de parámetros y actualizar ParametrosCompartidos
     * 
     * Lee un ParamsMessage de la mqueue con recepción bloqueante (espera hasta
     * que llegue un mensaje). Luego actualiza:
     * - params->kp, params->ki, params->kd (nuevas ganancias)
     * - params->setpoint (nueva referencia)
     * - params->signal_type (tipo de señal si aplica)
     * 
     * Los cambios se escriben con lock(params->mtx) para evitar carreras de datos.
     * 
     * @return true si recepción exitosa, false si hay error de comunicación
     * @note Esta función es bloqueante (espera mensaje)
     * @warning Debe llamarse solo después de inicializar()
     */
    bool recibir();
    
    /**
     * @brief Cerrar comunicación y liberar recursos
     * 
     * Desconecta de la mqueue y marca el receptor como no inicializado.
     */
    void cerrar();
    
    /**
     * @brief Verificar si el receptor está inicializado y listo
     * @return true si está inicializado y puede recibir parámetros
     */
    bool estaInicializado() const { return inicializado_; }

private:
    ParametrosCompartidos* params_;              ///< Puntero a estructura de parámetros a actualizar
    std::unique_ptr<MQueueComm> comm_;           ///< Comunicación IPC (mqueue)
    bool inicializado_;                          ///< Flag de inicialización
};

#endif // RECEPTOR_H
