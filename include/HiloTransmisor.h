/**
 * @file HiloTransmisor.h
 * @brief Threading para envío periódico de datos de control via IPC con temporización absoluta
 * @author Jordi + GitHub Copilot
 * @date 2026-01-03
 * 
 * Implementa un hilo POSIX que ejecuta el envío de datos de control
 * hacia la mqueue a una frecuencia configurable usando Temporizador
 * para evitar drift, permitiendo visualización en tiempo real sin bloquear
 * el lazo de control.
 */

#ifndef HILO_TRANSMISOR_H
#define HILO_TRANSMISOR_H

#include <pthread.h>
#include <csignal>
#include "Transmisor.h"

// Variable de control global para manejo de señales
extern volatile sig_atomic_t g_signal_run;

/**
 * @class HiloTransmisor
 * @brief Hilo dedicado para envío periódico de datos de control via IPC
 * 
 * Ejecuta un objeto Transmisor en un hilo pthread separado a frecuencia fija.
 * Esto permite que la GUI reciba muestras en tiempo real del comportamiento
 * del lazo de control sin interferir con la ejecución de los sistemas.
 * 
 * Diagrama de flujo:
 * @verbatim
 *   Control (control_simulator)       GUI (gui_app)
 *        │                              │
 *        ├─ VariablesCompartidas       │
 *        │  (ref, u, yk)               │
 *        │     │                        │
 *        │     v                        │
 *        │  HiloTransmisor              │
 *        │     │                        │
 *        │     └─> Transmisor::enviar() │
 *        │           │                  │
 *        │           v                  │
 *        │         mqueue ─────> DataMessage
 *        │           │                  │
 *        │           │            Recibir & Visualizar
 * @endverbatim
 * 
 * Patrón de uso (en control_simulator):
 * @code{.cpp}
 * VariablesCompartidas vars;
 * Transmisor tx(&vars);
 * if (tx.inicializar()) {
 *     pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
 *     
 *     HiloTransmisor hiloTx(&tx, &vars.running, &mtx, 50.0);  // 50 Hz
 *     
 *     // El hilo envía datos automáticamente
 *     // GUI recibe a través de recepción bloqueante
 *     vars.running = false; // Señal para detener
 * }
 * @endcode
 * 
 * @invariant frequency_ > 0 (Hz)
 * @invariant El hilo solo ejecuta transmisor->enviar() mientras *running_ == true
 */
class HiloTransmisor {
public:
    /**
     * @brief Constructor que crea e inicia el hilo de envío
     * 
     * @param transmisor Puntero al objeto Transmisor inicializado
     * @param running Puntero a variable booleana de control
     * @param mtx Puntero al mutex POSIX compartido
     * @param frequency Frecuencia de envío en Hz (período = 1/frequency)
     * 
     * @note El hilo comienza a ejecutarse inmediatamente
     * @note El Transmisor debe estar ya inicializado antes de pasar a este constructor
     * @note La frecuencia típicamente es menor que la del lazo (para no saturar mqueue)
     */
    HiloTransmisor(Transmisor* transmisor, bool* running, 
                   pthread_mutex_t* mtx, double frequency);
    
    /**
     * @brief Destructor que espera terminación del hilo
     */
    ~HiloTransmisor();
    
    /**
     * @brief Obtiene el identificador del hilo pthread
     * @return pthread_t ID del hilo
     */
    pthread_t getThread() const { return thread_; }

private:
    /**
     * @brief Función estática para pthread_create
     * @param arg Puntero a la instancia HiloTransmisor
     * @return nullptr
     */
    static void* threadFunc(void* arg);
    
    /**
     * @brief Loop principal del hilo
     * 
     * Ejecuta transmisor->enviar() a frecuencia fija mientras
     * *running_ sea true. Lee running bajo protección mutex.
     */
    void run();

    Transmisor* transmisor_;        ///< Puntero al transmisor
    bool* running_;                 ///< Flag de ejecución
    pthread_mutex_t* mtx_;          ///< Mutex POSIX compartido
    double frequency_;              ///< Frecuencia de envío (Hz)
    pthread_t thread_;              ///< ID del hilo pthread
};

#endif // HILO_TRANSMISOR_H
