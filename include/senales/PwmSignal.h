#pragma once

#include "senales/Signal.h"

namespace SignalGenerator {

/**
 * @brief Señal PWM (Pulse Width Modulation)
 * 
 * Genera una señal de pulsos rectangulares con ciclo de trabajo configurable.
 */
class PwmSignal : public Signal {
    double amplitude_;
    double duty_;
    double period_;

public:
    /**
     * @brief Construye una señal PWM.
     * @param Ts Periodo de muestreo [s].
     * @param amplitude Amplitud de pulso.
     * @param duty Ciclo de trabajo [0..1].
     * @param period Periodo de la PWM en segundos.
     * @param offset Desplazamiento vertical.
     * @param buffer_size Tamaño del buffer.
     */
    PwmSignal(double Ts, double amplitude, double duty, double period,
              double offset = 0.0, std::size_t buffer_size = 1024);

    /**
     * @brief Calcula el valor de la PWM en un tiempo dado.
     * @param time Tiempo en segundos.
     */
    double computeAt(double time) const override;

    /**
     * @note Diseño: PwmSignal muestra cómo encapsular parámetros (duty, period)
     * y validar en el constructor para evitar estados inválidos durante el uso.
     */

    double& amplitude();
    const double& amplitude() const;

    double& duty();
    const double& duty() const;

    double& period();
    const double& period() const;
};

} // namespace SignalGenerator
