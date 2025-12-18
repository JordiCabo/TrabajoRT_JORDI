/**
 * @file StateSpaceSystem.h
 * @brief Sistema discreto definido en espacio de estados
 * @author Jordi + GitHub Copilot
 * @date 2025-10-26
 */

#ifndef DISCRETESYSTEMS_STATESPACESYSTEM_H
#define DISCRETESYSTEMS_STATESPACESYSTEM_H

#include "DiscreteSystem.h"
#include <vector>

namespace DiscreteSystems {

/**
 * @class StateSpaceSystem
 * @brief Sistema discreto SISO en espacio de estados
 * 
 * El sistema se define mediante las ecuaciones:
 * 
 * x(k+1) = A*x(k) + B*u(k)
 * y(k)   = C*x(k) + D*u(k)
 * 
 * Donde:
 * - x(k) ∈ R^n es el vector de estado
 * - u(k) ∈ R es la entrada (escalar, SISO)
 * - y(k) ∈ R es la salida (escalar, SISO)
 * - A ∈ R^(n×n) es la matriz de estado
 * - B ∈ R^n es el vector de entrada
 * - C ∈ R^n es el vector de salida
 * - D ∈ R es la ganancia directa (escalar)
 * 
 * @invariant A_.size() == n (filas)
 * @invariant A_[i].size() == n (columnas) para todo i
 * @invariant B_.size() == n
 * @invariant C_.size() == n
 * @invariant x_.size() == n
 */
class StateSpaceSystem : public DiscreteSystem {
public:
    /**
     * @brief Constructor
     * @param A Matriz de estado n×n (vector de vectores)
     * @param B Vector de entrada de tamaño n
     * @param C Vector de salida de tamaño n
     * @param D Ganancia directa (escalar)
     * @param Ts Período de muestreo (debe ser > 0)
     * @param bufferSize Tamaño del buffer circular (por defecto 100)
     * @throws InvalidDimensions si las dimensiones son inconsistentes
     * @throws InvalidSamplingTime si Ts <= 0
     * 
     * Validaciones realizadas:
     * - A debe ser cuadrada (n×n con n > 0)
     * - B y C deben tener tamaño n
     */
    StateSpaceSystem(const std::vector<std::vector<double>>& A,
                    const std::vector<double>& B,
                    const std::vector<double>& C,
                    double D,
                    double Ts,
                    size_t bufferSize = 100);

    /**
     * @brief Obtiene la matriz A
     * @return Referencia constante a la matriz de estado
     */
    const std::vector<std::vector<double>>& getA() const { return A_; }

    /**
     * @brief Obtiene el vector B
     * @return Referencia constante al vector de entrada
     */
    const std::vector<double>& getB() const { return B_; }

    /**
     * @brief Obtiene el vector C
     * @return Referencia constante al vector de salida
     */
    const std::vector<double>& getC() const { return C_; }

    /**
     * @brief Obtiene la ganancia directa D
     * @return Valor de D
     */
    double getD() const { return D_; }

    /**
     * @brief Obtiene el vector de estado actual
     * @return Referencia constante al vector de estado x(k)
     */
    const std::vector<double>& getState() const { return x_; }

protected:
    /**
     * @brief Calcula la salida del sistema mediante las ecuaciones de estado
     * 
     * 1. Calcula y(k) = C*x(k) + D*u(k)
     * 2. Actualiza el estado: x(k+1) = A*x(k) + B*u(k)
     * 
     * @param uk Entrada en el paso k
     * @return Salida y(k)
     */
    double compute(double uk) override;

    /**
     * @brief Reinicia el estado interno (vector x a cero)
     */
    void resetState() override;

private:
    std::vector<std::vector<double>> A_;  ///< Matriz de estado n×n
    std::vector<double> B_;               ///< Vector de entrada (tamaño n)
    std::vector<double> C_;               ///< Vector de salida (tamaño n)
    double D_;                            ///< Ganancia directa (escalar)
    std::vector<double> x_;               ///< Vector de estado actual x(k)
    size_t n_;                            ///< Orden del sistema (dimensión de x)
};

/**
 * @brief Sobrecarga del operador << para imprimir los parámetros del sistema
 * @param os Stream de salida
 * @param sys Sistema a imprimir
 * @return Referencia al stream
 */
std::ostream& operator<<(std::ostream& os, const StateSpaceSystem& sys);

} // namespace DiscreteSystems

#endif // DISCRETESYSTEMS_STATESPACESYSTEM_H