/**
 * @file DAConverter.h
 * @brief Convertidor Digital-Analógico con retenedor de orden cero (ZOH)
 * @author Jordi + GitHub Copilot
 * @date 2025-12-18
 * 
 * Implementa un conversor D/A que mantiene la entrada durante un período
 * de muestreo (Zero-Order Hold), simulando un retenedor real.
 */

#ifndef DISCRETESYSTEMS_DACONVERTER_H
#define DISCRETESYSTEMS_DACONVERTER_H

#include "DiscreteSystem.h"
#include <iostream>

namespace DiscreteSystems {

/**
 * @class DAConverter
 * @brief Convertidor Digital-Analógico con retenedor de orden cero (ZOH)
 * 
 * Implementa un convertidor D/A ideal (sin retardo) que produce una salida
 * igual a la entrada. La salida se mantiene constante durante un período
 * de muestreo, simulando un muestreador ZOH típico:
 * 
 *   y(k) = u(k)
 * 
 * @invariant u_out_ siempre contiene la última entrada procesada u(k)
 */
class DAConverter : public DiscreteSystem {
public:
    /**
     * @brief Constructor del convertidor D/A
     * @param Ts Período de muestreo en segundos (debe ser > 0)
     * @param bufferSize Tamaño del buffer circular de muestras (por defecto 100)
     * @throws InvalidSamplingTime si Ts <= 0
     * 
     * @note Inicializa u_out_ = 0 para simular salida previa nula
     */
    explicit DAConverter(double Ts, size_t bufferSize = 100);

    /**
     * @brief Obtiene el último valor convertido de salida
     * @return Valor u[k] de salida actual
     */
    double getLastOutput() const { return u_out_; }

protected:
    /**
     * @brief Calcula la salida del convertidor D/A (ZOH)
     * 
     * Implementa el retenedor de orden cero: y(k) = u(k), almacenando
     * la entrada para mantenerla constante durante el período de muestreo.
     * 
     * @param uk Entrada digital en el paso k
     * @return Valor de salida u(k) (igual a la entrada)
     * 
     * @invariant Después de ejecutar, u_out_ = uk
     */
    double compute(double uk) override;

    /**
     * @brief Reinicia el estado interno del convertidor
     * 
     * Establece u_out_ = 0, preparando el sistema para una nueva
     * simulación desde condiciones iniciales.
     */
    void resetState() override;

private:
    double u_out_; ///< Valor de salida actual u[k]
};

} // namespace DiscreteSystems

#endif // DISCRETESYSTEMS_DACONVERTER_H
