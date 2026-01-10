/**
 * @file Hilo2in.h
 * @brief Wrapper de threading para ejecutar sistemas discretos con dos entradas en tiempo real
 * @author Jordi + GitHub Copilot
 * @date 2026-01-10
 * 
 * Proporciona ejecución pthread de un sistema discreto a una frecuencia fija,
 * con soporte para dos entradas independientes y protección de variables mediante mutex.
 * 
 * Utiliza shared_ptr para garantizar ciclo de vida seguro de objetos compartidos
 * entre múltiples hilos.
 */

#pragma once
#include <pthread.h>
#include <iostream>
#include <unistd.h>
#include <mutex>
#include <csignal>
#include <memory>
#include "DiscreteSystem.h"

// Variable de control global para manejo de señales
extern volatile sig_atomic_t g_signal_run;

namespace DiscreteSystems {

/**
 * @class Hilo2in
 * @brief Ejecutor en tiempo real de sistemas discretos con two-input cycle management seguro
 * 
 * Similar a Hilo, pero permite especificar dos punteros de entrada independientes.
 * Útil para sistemas como Sumador que requieren múltiples entradas.
 * 
 * Utiliza shared_ptr para variables compartidas, garantizando que:
 * - El sistema discreto no se destruye mientras el hilo está activo
 * - Las variables de entrada/salida existen durante el acceso
 * - El mutex es válido mientras hay operaciones en progreso
 * 
 * Patrón de uso (v1.0.4+):
 * @code{.cpp}
 * auto system = std::make_shared<DiscreteSystems::Sumador>(Ts);
 * auto ref = std::make_shared<double>(1.0);
 * auto feedback = std::make_shared<double>(0.0);
 * auto error = std::make_shared<double>(0.0);
 * auto running = std::make_shared<bool>(true);
 * auto mtx = std::make_shared<pthread_mutex_t>();
 * pthread_mutex_init(mtx.get(), nullptr);
 * 
 * DiscreteSystems::Hilo2in thread(system, ref, feedback, error, running, mtx, 100); // 100 Hz
 * @endcode
 * 
 * @invariant El hilo solo accede a *input1_, *input2_ y *output_ dentro de secciones protegidas por mtx
 * @invariant frequency_ > 0 (Hz)
 * @invariant Propiedad compartida: shared_ptr garantiza validez hasta destrucción del hilo
 */
class Hilo2in {
public:
    /**
     * @brief Constructor que inicia la ejecución del hilo con dos entradas
     * 
     * @param system shared_ptr al sistema discreto a ejecutar (debe soportar dos entradas)
     * @param input1 shared_ptr a la primera variable de entrada del sistema
     * @param input2 shared_ptr a la segunda variable de entrada del sistema
     * @param output shared_ptr a la variable de salida del sistema
     * @param running shared_ptr a variable booleana de control; cuando es false, el hilo se detiene
     * @param mtx shared_ptr al mutex POSIX que protege las variables compartidas
     * @param frequency Frecuencia de ejecución en Hz (período = 1/frequency)
     * 
     * @note El hilo comienza a ejecutarse inmediatamente desde el constructor
     * @note Para sistemas como Sumador, input1 es la referencia e input2 es la realimentación
     * @note shared_ptr incrementa el contador de referencias; hilo mantiene co-propiedad
     */
    Hilo2in(std::shared_ptr<DiscreteSystem> system, 
            std::shared_ptr<double> input1, 
            std::shared_ptr<double> input2, 
            std::shared_ptr<double> output,
            std::shared_ptr<bool> running, 
            std::shared_ptr<pthread_mutex_t> mtx, 
            double frequency=100);

    /**
     * @brief Obtiene el identificador del hilo pthread
     * @return pthread_t ID del hilo
     */
    pthread_t getThread() const { return thread_; }

    /**
     * @brief Destructor que espera a que termine el hilo
     * 
     * Ejecuta pthread_join() para asegurar que el hilo finaliza
     * correctamente antes de destruir el objeto.
     * shared_ptr decrementa referencias automáticamente.
     */
    ~Hilo2in();

private:
    std::shared_ptr<DiscreteSystem> system_;    ///< Co-propiedad del sistema a ejecutar
    std::shared_ptr<double> input1_;            ///< Co-propiedad de primera variable de entrada compartida
    std::shared_ptr<double> input2_;            ///< Co-propiedad de segunda variable de entrada compartida
    std::shared_ptr<double> output_;            ///< Co-propiedad de variable de salida compartida
    std::shared_ptr<pthread_mutex_t> mtx_;      ///< Co-propiedad del mutex POSIX para sincronización
    std::shared_ptr<bool> running_;             ///< Co-propiedad de variable de control de ejecución
    
    int iterations_;            ///< Número de iteraciones a ejecutar
    double frequency_;          ///< Frecuencia de ejecución en Hz
    pthread_t thread_;          ///< Identificador del hilo pthread

    /**
     * @brief Función estática de punto de entrada del hilo
     * 
     * @param arg Puntero a this (el objeto Hilo2in)
     * @return nullptr
     * 
     * @note Esta es la función que pthread llama; internamente invoca run()
     */
    static void* threadFunc(void* arg);

    /**
     * @brief Loop principal de ejecución del hilo
     * 
     * Ejecuta el sistema en bucle a la frecuencia especificada mientras
     * *running_ sea true, pasando ambas entradas. Sincroniza entrada/salida
     * con el mutex.
     */
    void run();
};

} // namespace DiscreteSystems
