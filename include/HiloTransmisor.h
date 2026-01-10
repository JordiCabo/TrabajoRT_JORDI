/**
 * @file HiloTransmisor.h
 * @brief Threading para envío periódico de datos de control via IPC con temporización absoluta
 * @author Jordi + GitHub Copilot
 * @date 2026-01-10
 * 
 * Implementa un hilo POSIX que ejecuta el envío de datos de control
 * hacia la mqueue a una frecuencia configurable usando Temporizador
 * para evitar drift, permitiendo visualización en tiempo real sin bloquear
 * el lazo de control. Utiliza shared_ptr para ciclo de vida seguro.
 */

#ifndef HILO_TRANSMISOR_H
#define HILO_TRANSMISOR_H

#include <pthread.h>
#include <memory>
#include <csignal>
#include "Transmisor.h"

// Variable de control global para manejo de señales
extern volatile sig_atomic_t g_signal_run;

/**
 * @class HiloTransmisor
 * @brief Hilo dedicado para envío periódico de datos de control via IPC (con shared_ptr)
 * 
 * Ejecuta un objeto Transmisor en un hilo pthread separado a frecuencia fija.
 * Esto permite que la GUI reciba muestras en tiempo real del comportamiento
 * del lazo de control sin interferir con la ejecución de los sistemas.
 * 
 * Utiliza shared_ptr para garantizar ciclo de vida seguro: el Transmisor,
 * flag de ejecución y mutex permanecen vivos mientras el hilo los necesita.
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
 *        │     │  (shared_ptr)          │
 *        │     └─> Transmisor::enviar() │
 *        │           │                  │
 *        │           v                  │
 *        │         mqueue ─────> DataMessage
 *        │           │                  │
 *        │           │            Recibir & Visualizar
 * @endverbatim
 * 
 * Patrón de uso (v1.0.4+ - con shared_ptr):
 * @code{.cpp}
 * auto transmisor = std::make_shared<Transmisor>();
 * auto running = std::make_shared<bool>(true);
 * auto mtx = std::make_shared<pthread_mutex_t>();
 * pthread_mutex_init(mtx.get(), nullptr);
 * 
 * if (transmisor->inicializar()) {
 *     HiloTransmisor hiloTx(transmisor, running, mtx, 50.0);  // 50 Hz
 *     // El hilo envía datos automáticamente
 *     // El shared_ptr co-propiedad garantiza ciclo de vida seguro
 * }
 * @endcode
 * 
 * @invariant frequency_ > 0 (Hz)
 * @invariant El hilo solo ejecuta transmisor->enviar() mientras *running_ == true
 * @version v1.0.4+ - Migrado a shared_ptr para ciclo de vida seguro
 */
class HiloTransmisor {
public:
    /**
     * @brief Constructor que establece co-propiedad compartida
     * 
     * @param transmisor shared_ptr al objeto Transmisor inicializado
     * @param running shared_ptr al flag de ejecución
     * @param mtx shared_ptr al mutex POSIX compartido
     * @param frequency Frecuencia de envío en Hz (período = 1/frequency)
     * 
     * @version v1.0.4+ - Usa shared_ptr para ciclo de vida seguro
     * 
     * La co-propiedad (co-ownership) del shared_ptr significa:
     * - El hilo mantiene su propia referencia al Transmisor, running y mtx
     * - Si el propietario original los destruye, permanecen vivos
     * - Cuando el hilo termina (destructor) baja el ref count
     * - Los objetos se destruyen solo cuando ref count = 0
     * 
     * @note El hilo comienza a ejecutarse inmediatamente
     * @note El Transmisor debe estar ya inicializado antes de pasar a este constructor
     * @note La frecuencia típicamente es menor que la del lazo (para no saturar mqueue)
     */
    HiloTransmisor(std::shared_ptr<Transmisor> transmisor, 
                   std::shared_ptr<bool> running, 
                   std::shared_ptr<pthread_mutex_t> mtx, 
                   double frequency);
    
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
     * Usa .get() para obtener punteros crudos para POSIX mutex API.
     */
    void run();

    std::shared_ptr<Transmisor> transmisor_;        ///< Transmisor con co-propiedad
    std::shared_ptr<bool> running_;                 ///< Flag de ejecución con co-propiedad
    std::shared_ptr<pthread_mutex_t> mtx_;          ///< Mutex POSIX con co-propiedad
    double frequency_;                               ///< Frecuencia de envío (Hz)
    pthread_t thread_;                               ///< ID del hilo pthread
};

#endif // HILO_TRANSMISOR_H
