/**
 * @file ADConverter.h
 * @brief Convertidor Analógico-Digital con retardo de muestreo
 * @author Jordi + GitHub Copilot
 * @date 2025-12-18
 * 
 * Simula un conversor A/D ideal que introduce un retardo de un período
 * de muestreo (características de un muestreador real).
 */

#ifndef DISCRETESYSTEMS_ADCONVERTER_H
#define DISCRETESYSTEMS_ADCONVERTER_H

#include "DiscreteSystem.h"
#include <iostream>

namespace DiscreteSystems {

/**
 * @class ADConverter
 * @brief Convertidor Analógico-Digital (muestreador con retardo)
 * 
 * Implementa un convertidor A/D que muestrea la entrada pero produce
 * la salida con un retardo de un período de muestreo:
 * 
 *   y(k) = u(k-1)
 * 
 * Esto simula el comportamiento de un muestreador real.
 * 
 * @invariant u_prev_ contiene el valor u(k-1) del paso anterior
 */
class ADConverter : public DiscreteSystem {
public:
    /**
     * @brief Constructor del convertidor A/D
     * @param Ts Período de muestreo en segundos (debe ser > 0)
     * @param bufferSize Tamaño del buffer circular de muestras (por defecto 100)
     * @throws InvalidSamplingTime si Ts <= 0
     * 
     * @note Inicializa u_prev_ = 0 para simular entrada previa nula
     */
    explicit ADConverter(double Ts, size_t bufferSize = 100);

    /**
     * @brief Obtiene el último valor de entrada almacenado
     * @return Valor u[k-1] del paso anterior
     */
    double getLastInput() const { return u_prev_; }

protected:
    /**
     * @brief Calcula la salida del convertidor A/D (ecuación en diferencias)
     * 
     * Implementa y(k) = u(k-1), guardando la entrada actual para el
     * próximo cálculo.
     * 
     * @param uk Entrada muestreada en el paso k
     * @return Valor u(k-1) del paso anterior
     * 
     * @invariant Después de ejecutar, u_prev_ = uk para el próximo paso
     */
    double compute(double uk) override;

    /**
     * @brief Reinicia el estado interno del convertidor
     * 
     * Establece u_prev_ = 0, preparando el sistema para una nueva
     * simulación desde condiciones iniciales.
     */
    void resetState() override;

private:
    double u_prev_; ///< Valor de entrada del paso anterior u[k-1]
};

} // namespace DiscreteSystems

#endif // DISCRETESYSTEMS_ADCONVERTER_H