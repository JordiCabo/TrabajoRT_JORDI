/*
 * @file HiloReceptor.h
 * 
 * @author Jordi
 * @author GitHub Copilot (asistencia)
 * 
 * @brief Wrapper de threading para recepción IPC periódica
 * 
 * Ejecuta el método recibir() de un Receptor en un hilo pthread
 * a frecuencia fija, permitiendo recepción periódica de parámetros
 * sin bloquear el hilo principal.
 * 
 * Uso típico:
 *   ParametrosCompartidos params;
 *   Receptor rx(&params);
 *   rx.inicializar();
 *   HiloReceptor hiloRx(&rx, &running, &mtx, 50.0); // 50 Hz
 *   // El hilo recibe automáticamente a 50 Hz
 *   running = false; // Detener
 */

#ifndef HILO_RECEPTOR_H
#define HILO_RECEPTOR_H

#include <pthread.h>
#include "Receptor.h"

/**
 * @class HiloReceptor
 * @brief Ejecuta recepción IPC periódica en hilo separado
 * 
 * Envuelve un objeto Receptor y lo ejecuta en un hilo pthread
 * a frecuencia configurable. Sincroniza el acceso mediante el
 * mutex compartido.
 */
class HiloReceptor {
public:
    /**
     * @brief Constructor que crea e inicia el hilo
     * @param receptor Puntero al receptor a ejecutar
     * @param running Puntero al flag de ejecución (bajo mutex)
     * @param mtx Puntero al mutex compartido POSIX
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

    Receptor* receptor_;            ///< Puntero al receptor
    bool* running_;                 ///< Flag de ejecución
    pthread_mutex_t* mtx_;          ///< Mutex POSIX compartido
    double frequency_;              ///< Frecuencia de recepción (Hz)
    pthread_t thread_;              ///< ID del hilo pthread
};

#endif // HILO_RECEPTOR_H
