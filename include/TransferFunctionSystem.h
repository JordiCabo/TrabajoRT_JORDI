/**
 * @file TransferFunctionSystem.h
 * @brief Sistema discreto definido mediante función de transferencia
 * @author Jordi + GitHub Copilot
 * @date 2025-10-26
 */

#ifndef DISCRETESYSTEMS_TRANSFERFUNCTIONSYSTEM_H
#define DISCRETESYSTEMS_TRANSFERFUNCTIONSYSTEM_H

#include "DiscreteSystem.h"
#include <vector>

namespace DiscreteSystems {

/**
 * @class TransferFunctionSystem
 * @brief Sistema discreto SISO definido por función de transferencia
 * 
 * La función de transferencia se define como:
 * 
 *         b[0] + b[1]*z^-1 + ... + b[m]*z^-m
 * H(z) = --------------------------------------
 *         a[0] + a[1]*z^-1 + ... + a[n]*z^-n
 * 
 * La ecuación en diferencias correspondiente es:
 * 
 * y(k) = (1/a[0]) * [b[0]*u(k) + b[1]*u(k-1) + ... + b[m]*u(k-m)
 *                    - a[1]*y(k-1) - ... - a[n]*y(k-n)]
 * 
 * Se normaliza internamente para que a[0] = 1.
 * 
 * @invariant a[0] != 0 (garantizado por normalización)
 * @invariant uHist_.size() == b_.size()
 * @invariant yHist_.size() == a_.size() - 1
 */
class TransferFunctionSystem : public DiscreteSystem {
public:
    /**
     * @brief Constructor
     * @param b Coeficientes del numerador [b0, b1, ..., bm]
     * @param a Coeficientes del denominador [a0, a1, ..., an]
     * @param Ts Período de muestreo (debe ser > 0)
     * @param bufferSize Tamaño del buffer circular (por defecto 100)
     * @throws InvalidCoefficients si a está vacío, b está vacío, o a[0] == 0
     * @throws InvalidSamplingTime si Ts <= 0
     * 
     * @note Los coeficientes se normalizan internamente para que a[0] = 1
     */
    TransferFunctionSystem(const std::vector<double>& b,
                          const std::vector<double>& a,
                          double Ts,
                          size_t bufferSize = 100);

    /**
     * @brief Obtiene los coeficientes del numerador
     * @return Vector de coeficientes b (ya normalizados)
     */
    const std::vector<double>& getNumerator() const { return b_; }

    /**
     * @brief Obtiene los coeficientes del denominador
     * @return Vector de coeficientes a (ya normalizados, con a[0] = 1)
     */
    const std::vector<double>& getDenominator() const { return a_; }

protected:
    /**
     * @brief Calcula la salida del sistema mediante la ecuación en diferencias
     * @param uk Entrada en el paso k
     * @return Salida y(k)
     */
    double compute(double uk) override;

    /**
     * @brief Reinicia el estado interno (historiales de entrada y salida)
     */
    void resetState() override;

private:
    std::vector<double> b_;       ///< Coeficientes del numerador (normalizados)
    std::vector<double> a_;       ///< Coeficientes del denominador (normalizados, a[0] = 1)
    std::vector<double> uHist_;   ///< Historial de entradas [u(k), u(k-1), ..., u(k-m)]
    std::vector<double> yHist_;   ///< Historial de salidas [y(k-1), y(k-2), ..., y(k-n)]
};

/**
 * @brief Sobrecarga del operador << para imprimir los parámetros del sistema
 * @param os Stream de salida
 * @param sys Sistema a imprimir
 * @return Referencia al stream
 */
//std::ostream& operator<<(std::ostream& os, const TransferFunctionSystem& sys);

} // namespace DiscreteSystems

#endif // DISCRETESYSTEMS_TRANSFERFUNCTIONSYSTEM_H
