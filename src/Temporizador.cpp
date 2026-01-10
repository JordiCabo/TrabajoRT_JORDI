/**
 * @file Temporizador.cpp
 * @brief Implementación del temporizador de retardo absoluto
 * @author Jordi + GitHub Copilot
 * @date 2026-01-10
 */

#include "../include/Temporizador.h"

namespace DiscreteSystems {

/**
 * @brief Constructor principal desde frecuencia en Hz
 */
Temporizador::Temporizador(double frequency) {
    // Calcular período en nanosegundos
    period_ns_ = static_cast<long>((1.0 / frequency) * 1e9);
    
    // Obtener tiempo actual como punto de partida
    clock_gettime(CLOCK_MONOTONIC, &next_);
}

/**
 * @brief Constructor estático desde período en segundos
 */
Temporizador Temporizador::desdePeriodo(double Ts) {
    return Temporizador(1.0 / Ts);
}

/**
 * @brief Espera hasta el siguiente período absoluto
 */
int Temporizador::esperar() {
    // Incrementar tiempo objetivo en un período
    next_.tv_nsec += period_ns_;
    
    // Manejar overflow de nanosegundos (si >= 1 segundo)
    if (next_.tv_nsec >= 1000000000L) {
        next_.tv_sec++;
        next_.tv_nsec -= 1000000000L;
    }
    
    // Dormir hasta el instante absoluto calculado
    return clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_, NULL);
}

/**
 * @brief Reinicia el temporizador al instante actual
 */
void Temporizador::reiniciar() {
    clock_gettime(CLOCK_MONOTONIC, &next_);
}

} // namespace DiscreteSystems
