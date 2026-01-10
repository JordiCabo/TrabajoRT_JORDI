/**
 * @file Temporizador.h
 * @brief Temporizador de retardo absoluto para hilos en tiempo real
 * @author Jordi + GitHub Copilot
 * @date 2026-01-10
 * 
 * Implementa temporización con clock_nanosleep TIMER_ABSTIME para evitar
 * drift acumulativo en loops de control periódico.
 */

#pragma once
#include <time.h>

namespace DiscreteSystems {

/**
 * @class Temporizador
 * @brief Gestiona retardos absolutos con clock_nanosleep para sistemas en tiempo real
 * 
 * Utiliza CLOCK_MONOTONIC con TIMER_ABSTIME para garantizar períodos exactos
 * sin acumulación de error de tiempo (drift). Ideal para loops de control
 * periódicos en hilos pthread.
 * 
 * Patrón de uso:
 * @code{.cpp}
 * Temporizador timer(100.0); // 100 Hz
 * 
 * while (running) {
 *     // Hacer trabajo de control
 *     processSample();
 *     
 *     // Dormir hasta el siguiente período
 *     timer.esperar();
 * }
 * @endcode
 * 
 * @invariant next_ siempre apunta al siguiente instante absoluto de activación
 * @invariant No acumula error de temporización entre ciclos
 */
class Temporizador {
private:
    struct timespec next_;      ///< Tiempo absoluto del próximo despertar
    long period_ns_;            ///< Período en nanosegundos
    
public:
    /**
     * @brief Constructor que inicializa el temporizador con frecuencia dada
     * 
     * @param frequency Frecuencia de muestreo en Hz (debe ser > 0)
     * 
     * Obtiene el tiempo actual con clock_gettime(CLOCK_MONOTONIC) y
     * calcula el período en nanosegundos para uso posterior.
     * 
     * @pre frequency > 0
     */
    explicit Temporizador(double frequency);
    
    /**
     * @brief Constructor alternativo que acepta período de muestreo en segundos
     * 
     * @param Ts Período de muestreo en segundos (debe ser > 0)
     * 
     * @pre Ts > 0
     */
    static Temporizador desdePeriodo(double Ts);
    
    /**
     * @brief Espera hasta el siguiente período (retardo absoluto)
     * 
     * Incrementa next_ en period_ns_ y duerme hasta ese instante absoluto
     * usando clock_nanosleep con TIMER_ABSTIME. Maneja overflow de nanosegundos
     * automáticamente.
     * 
     * @return 0 si exitoso, código de error POSIX si falla
     * 
     * @post next_ avanza exactamente un período
     * @post No acumula drift de temporización
     */
    int esperar();
    
    /**
     * @brief Reinicia el temporizador al instante actual
     * 
     * Útil para resincronizar después de operaciones largas o cambios de modo.
     * Obtiene el tiempo actual y lo asigna a next_.
     */
    void reiniciar();
};

} // namespace DiscreteSystems
