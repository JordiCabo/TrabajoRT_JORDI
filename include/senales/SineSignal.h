#pragma once

#include "senales/Signal.h"

namespace SignalGenerator {

/**
 * @brief Señal sinusoidal
 * 
 * Genera una señal seno con amplitud, frecuencia y fase configurables.
 */
class SineSignal : public Signal {
    double amplitude_;
    double freq_;
    double phase_;

public:
    /**
     * @brief Construye una señal sinusoidal.
     * @param Ts Periodo de muestreo [s].
     * @param amplitude Amplitud de la señal.
     * @param freq Frecuencia en Hz.
     * @param phase Fase inicial en radianes.
     * @param offset Desplazamiento vertical.
     * @param buffer_size Tamaño del buffer.
     */
    SineSignal(double Ts, double amplitude, double freq,
               double phase = 0.0, double offset = 0.0,
               std::size_t buffer_size = 1024);

    /**
     * @brief Calcula el valor seno en el tiempo dado.
     * @param time Tiempo en segundos.
     */
    double computeAt(double time) const override;

    /**
     * @note Diseño: SineSignal es un ejemplo de señal analítica; observar cómo
     * computeAt no modifica el estado, lo cual facilita el razonamiento.
     */

    double& amplitude();
    const double& amplitude() const;

    double& frequency();
    const double& frequency() const;

    double& phase();
    const double& phase() const;
};

} // namespace SignalGenerator
