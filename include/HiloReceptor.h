/**
 * @file HiloReceptor.h
 * @brief Threading para recepción periódica de parámetros via IPC con temporización absoluta (con shared_ptr)
 * @author Jordi + GitHub Copilot
 * @date 2026-01-10
 * 
 * Implementa un hilo POSIX que ejecuta la recepción de parámetros
 * desde la mqueue a una frecuencia configurable usando Temporizador,
 * desacoplando el proceso IPC de la ejecución del lazo de control.
 * Utiliza shared_ptr para ciclo de vida seguro.
 */

#ifndef HILO_RECEPTOR_H
#define HILO_RECEPTOR_H

#include <pthread.h>
#include <memory>
#include <csignal>
#include "Receptor.h"

// Variable de control global para manejo de señales
extern volatile sig_atomic_t g_signal_run;

/**
 * @class HiloReceptor
 * @brief Hilo dedicado para recepción periódica de parámetros via IPC (con shared_ptr)
 * 
 * Ejecuta un objeto Receptor en un hilo pthread separado a frecuencia fija.
 * Esto permite que la GUI pueda enviar actualizaciones de parámetros que se
 * reciben periódicamente sin bloquear el lazo de control principal.
 * 
 * Utiliza shared_ptr para garantizar ciclo de vida seguro: Receptor,
 * flag de ejecución y mutex permanecen vivos mientras el hilo los necesita.
 * 
 * Diagrama de flujo:
 * @verbatim
 *   GUI (gui_app)                Control (control_simulator)
 *        │                              │
 *        ├─ ParamsMessage ─────> mqueue ─────> HiloReceptor
 *        │                              │          │  (shared_ptr)
 *        │                              │          └─> Receptor::recibir()
 *        │                              │                   │
 *        │                              │                   v
 *        │                              │          ParametrosCompartidos
 *        │                              │          (lock → kp,ki,kd updated)
 * @endverbatim
 * 
 * Patrón de uso (v1.0.4+ - con shared_ptr):
 * @code{.cpp}
 * auto params = std::make_shared<ParametrosCompartidos>();
 * auto receptor = std::make_shared<Receptor>(params.get());
 * auto running = std::make_shared<bool>(true);
 * auto mtx = std::make_shared<pthread_mutex_t>();
 * pthread_mutex_init(mtx.get(), nullptr);
 * 
 * if (receptor->inicializar()) {
 *     HiloReceptor hiloRx(receptor, running, mtx, 50.0);  // 50 Hz
 *     // El hilo recibe parámetros automáticamente
 *     // El shared_ptr co-propiedad garantiza ciclo de vida seguro
 * }
 * @endcode
 * 
 * @invariant frequency_ > 0 (Hz)
 * @invariant El hilo solo ejecuta receptor->recibir() mientras *running_ == true
 * @version v1.0.4+ - Migrado a shared_ptr para ciclo de vida seguro
 */
class HiloReceptor {
public:
    /**
     * @brief Constructor que establece co-propiedad compartida
     * 
     * @param receptor shared_ptr al objeto Receptor inicializado
     * @param running shared_ptr al flag de ejecución
     * @param mtx shared_ptr al mutex POSIX compartido
     * @param frequency Frecuencia de recepción en Hz
     * 
     * @version v1.0.4+ - Usa shared_ptr para ciclo de vida seguro
     * 
     * La co-propiedad (co-ownership) del shared_ptr significa:
     * - El hilo mantiene su propia referencia al Receptor, running y mtx
     * - Si el propietario original los destruye, permanecen vivos
     * - Cuando el hilo termina (destructor) baja el ref count
     * - Los objetos se destruyen solo cuando ref count = 0
     * 
     * @note El hilo comienza a ejecutarse inmediatamente
     */
    HiloReceptor(std::shared_ptr<Receptor> receptor, 
                 std::shared_ptr<bool> running, 
                 std::shared_ptr<pthread_mutex_t> mtx, 
                 double frequency);
    
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
     * Usa .get() para obtener punteros crudos para POSIX mutex API.
     */
    void run();

    std::shared_ptr<Receptor> receptor_;            ///< Receptor con co-propiedad
    std::shared_ptr<bool> running_;                 ///< Flag de ejecución con co-propiedad
    std::shared_ptr<pthread_mutex_t> mtx_;          ///< Mutex POSIX con co-propiedad
    double frequency_;                               ///< Frecuencia de recepción (Hz)
    pthread_t thread_;                               ///< ID del hilo pthread
};

#endif // HILO_RECEPTOR_H
