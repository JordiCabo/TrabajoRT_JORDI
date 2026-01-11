/**
 * @file HiloReceptor.h
 * @brief Threading para recepción periódica de parámetros via IPC con temporización absoluta
 * @author Jordi + GitHub Copilot
 * @date 2026-01-03
 * 
 * Implementa un hilo POSIX que ejecuta la recepción de parámetros
 * desde la mqueue a una frecuencia configurable usando Temporizador,
 * desacoplando el proceso IPC de la ejecución del lazo de control.
 */

#ifndef HILO_RECEPTOR_H
#define HILO_RECEPTOR_H

#include <pthread.h>
#include <csignal>
#include <memory>
#include <atomic>
#include "Receptor.h"

// Variable de control global para manejo de señales
extern volatile sig_atomic_t g_signal_run;

/**
 * @class HiloReceptor
 * @brief Hilo dedicado para recepción periódica de parámetros via IPC
 * 
 * Ejecuta un objeto Receptor en un hilo pthread separado a frecuencia fija.
 * Esto permite que la GUI pueda enviar actualizaciones de parámetros que se
 * reciben periódicamente sin bloquear el lazo de control principal.
 * 
 * Diagrama de flujo:
 * @verbatim
 *   GUI (gui_app)                Control (control_simulator)
 *        │                              │
 *        ├─ ParamsMessage ─────> mqueue ─────> HiloReceptor
 *        │                              │          │
 *        │                              │          └─> Receptor::recibir()
 *        │                              │                   │
 *        │                              │                   v
 *        │                              │          ParametrosCompartidos
 *        │                              │          (lock → kp,ki,kd updated)
 * @endverbatim
 * 
 * Patrón de uso (en control_simulator):
 * @code{.cpp}
 * ParametrosCompartidos params;
 * Receptor rx(&params);
 * if (rx.inicializar()) {
 *     bool running = true;
 *     pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
 *     
 *     HiloReceptor hiloRx(&rx, &running, &mtx, 50.0);  // 50 Hz
 *     
 *     // El hilo recibe parámetros automáticamente
 *     running = false; // Señal para detener
 * }
 * @endcode
 * 
 * @invariant frequency_ > 0 (Hz)
 * @invariant El hilo solo ejecuta receptor->recibir() mientras *running_ == true
 */
class HiloReceptor {
public:
    /**
     * @brief Constructor con smart pointers (recomendado)
     * 
     * @param receptor Smart pointer al objeto Receptor inicializado
     * @param running Smart pointer a variable booleana de control
     * @param mtx Smart pointer al mutex POSIX compartido
     * @param frequency Frecuencia de recepción en Hz
     */
    HiloReceptor(std::shared_ptr<Receptor> receptor, 
                 std::shared_ptr<bool> running, 
                 std::shared_ptr<pthread_mutex_t> mtx, 
                 double frequency);
    
    /**
     * @brief Constructor con punteros crudos (compatibilidad)
     * @deprecated Usar constructor con smart pointers
     * 
     * @param receptor Puntero al objeto Receptor inicializado
     * @param running Puntero a variable booleana de control
     * @param mtx Puntero al mutex POSIX compartido
     * @param frequency Frecuencia de recepción en Hz
     */
    HiloReceptor(Receptor* receptor, bool* running, 
                 pthread_mutex_t* mtx, double frequency);
    
    /**
     * @brief Destructor que espera terminación del hilo
     */
    ~HiloReceptor();
    
    /**
     * @brief Obtiene el identificador del hilo pthread
     * @return pthread_t ID del hilo
     */
    pthread_t getThread() const { return thread_; }

private:
    /**
     * @brief Función estática para pthread_create
     * @param arg Puntero a la instancia HiloReceptor
     * @return nullptr
     */
    static void* threadFunc(void* arg);
    
    /**
     * @brief Loop principal del hilo
     * 
     * Ejecuta receptor->recibir() a frecuencia fija mientras
     * *running_ sea true. Lee running bajo protección mutex.
     */
    void run();

    // Smart pointers
    std::shared_ptr<Receptor> receptor_;
    std::shared_ptr<bool> running_;
    std::shared_ptr<pthread_mutex_t> mtx_;
    
    // Raw pointers (compatibilidad)
    Receptor* receptor_raw_;
    bool* running_raw_;
    pthread_mutex_t* mtx_raw_;
    
    double frequency_;              ///< Frecuencia de recepción (Hz)
    pthread_t thread_;              ///< ID del hilo pthread
};

#endif // HILO_RECEPTOR_H
