#pragma once

#include <pthread.h>
#include <memory>
#include "InterruptorArranque.h"

/**
 * @class HiloIntArranque
 * @brief Hilo dedicado para monitoreo de interruptor de arranque/paro con ciclo de vida seguro
 * 
 * Ejecuta un InterruptorArranque en un hilo pthread separado a frecuencia fija.
 * Utiliza shared_ptr para garantizar ciclo de vida seguro de objetos compartidos.
 * 
 * Patrón de uso (v1.0.4+):
 * @code{.cpp}
 * auto interruptor = std::make_shared<InterruptorArranque>();
 * auto running = std::make_shared<bool>(true);
 * auto mtx = std::make_shared<pthread_mutex_t>();
 * pthread_mutex_init(mtx.get(), nullptr);
 * 
 * HiloIntArranque hiloInterruptor(interruptor, running, mtx, 10.0); // 10 Hz
 * 
 * // El hilo monitorea automáticamente
 * *running = false; // Detiene el hilo
 * @endcode
 * 
 * @invariant frequency_ > 0 (Hz)
 * @invariant Propiedad compartida: shared_ptr garantiza validez hasta destrucción
 */
class HiloIntArranque {
private:
    std::shared_ptr<InterruptorArranque> interruptor_;  ///< Co-propiedad del interruptor
    std::shared_ptr<bool> running_;                     ///< Co-propiedad de flag de ejecución
    std::shared_ptr<pthread_mutex_t> mtx_;              ///< Co-propiedad del mutex POSIX
    pthread_t thread_;                                  ///< ID del hilo pthread
    double frequency_;                                  ///< Frecuencia de monitoreo (Hz)
    
    void run();
    static void* threadFunc(void* arg);

public:
    /**
     * @brief Constructor que crea e inicia el hilo de monitoreo
     * 
     * @param interruptor shared_ptr al InterruptorArranque
     * @param running shared_ptr a variable booleana de control
     * @param mtx shared_ptr al mutex POSIX compartido
     * @param frequency Frecuencia de monitoreo en Hz (default 10.0)
     * 
     * @note El hilo comienza a ejecutarse inmediatamente
     * @note shared_ptr incrementa el contador de referencias; hilo mantiene co-propiedad
     */
    HiloIntArranque(std::shared_ptr<InterruptorArranque> interruptor, 
                    std::shared_ptr<bool> running, 
                    std::shared_ptr<pthread_mutex_t> mtx, 
                    double frequency = 10.0);
    ~HiloIntArranque();
};
