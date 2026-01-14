/**
 * @file Discretizer.h
 * @brief Utilidades para discretizar funciones de transferencia continuas
 *        mediante Tustin (bilineal) u otros métodos futuros.
 */

#pragma once

#include <vector>
#include <stdexcept>

namespace DiscreteSystems {

/** Métodos de discretización disponibles */
enum class DiscretizationMethod {
    Tustin,   ///< Transformación bilineal (prewarping opcional en futuro)
    ZOH       ///< Zero-Order Hold (no implementado aún)
};

/** Resultado de una discretización: coeficientes en z^-1 */
struct DiscreteTF {
    std::vector<double> b; ///< Numerador discreto (orden descendente en z^-1)
    std::vector<double> a; ///< Denominador discreto (orden descendente en z^-1), a[0] = 1
};

/**
 * @brief Discretiza una función de transferencia continua B(s)/A(s)
 *
 * @param num_s Coeficientes de B(s) en orden descendente
 * @param den_s Coeficientes de A(s) en orden descendente (A[0] != 0)
 * @param Ts    Período de muestreo en segundos
 * @param method Método de discretización (solo Tustin implementado)
 * @return DiscreteTF coeficientes discretos en z^-1
 * @throws std::invalid_argument si Ts <= 0 o denominador inválido
 */
DiscreteTF discretizeTF(const std::vector<double>& num_s,
                        const std::vector<double>& den_s,
                        double Ts,
                        DiscretizationMethod method = DiscretizationMethod::Tustin);

} // namespace DiscreteSystems
