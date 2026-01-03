/*
 * @file HiloTransmisor.h
 * 
 * @author Jordi
 * @author GitHub Copilot (asistencia)
 * 
 * @brief Wrapper de threading para transmisión IPC periódica
 * 
 * Ejecuta el método enviar() de un Transmisor en un hilo pthread
 * a frecuencia fija, permitiendo envío periódico de datos de control
 * sin bloquear el hilo principal.
 * 
 * Uso típico:
 *   VariablesCompartidas vars;
 *   Transmisor tx(&vars);
 *   tx.inicializar();
 *   HiloTransmisor hiloTx(&tx, &vars.running, &vars.mtx, 50.0); // 50 Hz
 *   // El hilo envía automáticamente a 50 Hz
 *   vars.running = false; // Detener
 */

#ifndef HILO_TRANSMISOR_H
#define HILO_TRANSMISOR_H

#include <pthread.h>
#include "Transmisor.h"

/**
 * @class HiloTransmisor
 * @brief Ejecuta transmisión IPC periódica en hilo separado
 * 
 * Envuelve un objeto Transmisor y lo ejecuta en un hilo pthread
 * a frecuencia configurable. Sincroniza el acceso mediante el
 * mutex compartido.
 */
class HiloTransmisor {
public:
    /**
     * @brief Constructor que crea e inicia el hilo
     * @param transmisor Puntero al transmisor a ejecutar
     * @param running Puntero al flag de ejecución (bajo mutex)
     * @param mtx Puntero al mutex compartido POSIX
     * @param frequency Frecuencia de envío en Hz
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
