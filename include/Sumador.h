/**
 * @file Sumador.h
 * @brief Sumador de dos entradas (restador comparador)
 * @author Jordi + GitHub Copilot
 * @date 2025-12-18
 * 
 * Implementa un bloque sumador que calcula la diferencia entre una referencia
 * y una realimentación para generar el error: e(k) = ref(k) - y(k)
 */

#ifndef DISCRETESYSTEMS_SUMADOR_H
#define DISCRETESYSTEMS_SUMADOR_H

#include "DiscreteSystem.h"
#include <iostream>

namespace DiscreteSystems {

/**
 * @class Sumador
 * @brief Bloque sumador restador para cálculo de error
 * 
 * Calcula la salida como la diferencia entre dos entradas:
 *   e(k) = ref(k) - y(k)
 * 
 * Típicamente utilizado en esquemas de control para calcular el error
 * que alimenta al controlador PID. Nota que esta clase hereda de
 * DiscreteSystem pero requiere una sobrecarga especial del método
 * compute() para manejar dos entradas.
 * 
 * @invariant e_out_ siempre contiene ref - y del último cálculo
 */
class Sumador : public DiscreteSystem {
public:
    /**
     * @brief Constructor del sumador
     * @param Ts Período de muestreo en segundos (debe ser > 0)
     * @param bufferSize Tamaño del buffer circular de muestras (por defecto 100)
     * @throws InvalidSamplingTime si Ts <= 0
     * 
     * @note Inicializa e_out_ = 0 para simular salida previa nula
     */
    explicit Sumador(double Ts, size_t bufferSize = 100);

    /**
     * @brief Obtiene el último valor de error calculado
     * @return Valor e[k] = ref[k] - y[k]
     */
    double getLastOutput() const { return e_out_; }

    /**
     * @brief Calcula el error (función real con dos entradas)
     * @param ref Valor de referencia deseada
     * @param y Valor medido real del sistema
     * @return Error e(k) = ref - y
     * 
     * Esta es la función principal que debe usarse para calcular
     * la diferencia entre referencia y realimentación.
     */
    double compute(double ref, double y);

protected:
    /**
     * @brief Método compute() de una sola entrada NO VÁLIDO para Sumador
     * 
     * El Sumador requiere dos entradas. Usar este método lanzará
     * una excepción indicando que debe usarse compute(ref, y).
     * 
     * @param uk Entrada única (no válida)
     * @return Nunca retorna; lanza excepción
     * @throws std::runtime_error indicando uso incorrecto
     */
    double compute(double uk) override;

    /**
     * @brief Reinicia el estado interno del sumador
     * 
     * Establece e_out_ = 0, preparando el sistema para una nueva
     * simulación desde condiciones iniciales.
     */
    void resetState() override;

private:
    double e_out_; ///< Valor del error calculado e[k] = ref − y
};

} // namespace DiscreteSystems

#endif // DISCRETESYSTEMS_SUMADOR_H
