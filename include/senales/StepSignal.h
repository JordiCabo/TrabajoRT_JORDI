#pragma once

#include "senales/Signal.h"

namespace SignalGenerator {

/**
 * @brief Señal escalón (step signal)
 * 
 * Genera una señal que vale 0 antes de step_time y amplitude después.
 */
class StepSignal : public Signal {
    double amplitude_;
    double step_time_;

public:
    /**
     * @brief Construye una señal escalón.
     * @param Ts Periodo de muestreo [s].
     * @param amplitude Valor de la amplitud del escalón.
     * @param step_time Tiempo a partir del cual la señal toma la amplitud.
     * @param offset Desplazamiento vertical adicional.
     * @param buffer_size Tamaño del buffer.
     */
    StepSignal(double Ts, double amplitude, double step_time,
               double offset = 0.0, std::size_t buffer_size = 1024);

    /**
     * @brief Calcula el valor del escalón en un tiempo dado.
     * @param time Tiempo en segundos.
     */
    double computeAt(double time) const override;

    /**
     * @note Diseño: StepSignal ilustra una subclase concreta que sólo implementa
     * la fórmula matemática. No gestiona buffers; esa responsabilidad es de
     * la clase base.
     */

    double& amplitude();
    const double& amplitude() const;

    double& stepTime();
    const double& stepTime() const;
};

} // namespace SignalGenerator
