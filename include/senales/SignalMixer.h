#pragma once

#include "senales/Signal.h"
#include <vector>
#include <memory>

namespace SignalGenerator {

/**
 * @brief Mezclador de señales (suma ponderada)
 * 
 * Combina múltiples señales mediante una suma ponderada.
 * Demuestra el patrón de composición: contiene referencias a otras señales.
 */
class SignalMixer : public Signal {
    std::vector<std::shared_ptr<Signal>> signals_;
    std::vector<double> weights_;

public:
    SignalMixer(double Ts,
                std::vector<std::shared_ptr<Signal>> signals,
                std::vector<double> weights = {},
                double offset = 0.0,
                std::size_t buffer_size = 1024);

    /**
     * @brief Calcula el valor mezclado en un tiempo dado (sin efectos).
     * @param time Tiempo en segundos.
     * @return Suma ponderada de las salidas de las señales internas.
     *
     * @note Diseño: SignalMixer demuestra composición: contiene punteros a
     * otras señales y combina sus salidas. Usamos std::shared_ptr porque la
     * propiedad puede ser compartida por distintos componentes. Si la
     * propiedad fuera única, sería preferible std::unique_ptr.
     */
    double computeAt(double time) const override;

    std::vector<std::shared_ptr<Signal>>& signals();
    const std::vector<std::shared_ptr<Signal>>& signals() const;

    std::vector<double>& weights();
    const std::vector<double>& weights() const;

    /**
     * @brief Avanza todas las señales internas (llama a next() en cada una)
     * y almacena la mezcla en el buffer propio.
     * @return Valor de la mezcla en la muestra avanzada.
     */
    double next() override;
};

} // namespace SignalGenerator
