/**
 * @file StateSpaceSystem.cpp
 * @brief Implementación del sistema discreto en espacio de estados
 * @author Jordi + GitHub Copilot
 * @date 2025-12-18
 */

#include "StateSpaceSystem.h"
#include <stdexcept>
#include <iomanip>

namespace DiscreteSystems {

/**
 * @brief Constructor del sistema en espacio de estados
 * @throws std::runtime_error si las dimensiones son inconsistentes
 * 
 * Valida que:
 * - A sea una matriz cuadrada n×n con n > 0
 * - B y C tengan tamaño exactamente n
 * - Ts sea válido
 */
StateSpaceSystem::StateSpaceSystem(
        const std::vector<std::vector<double>>& A,
        const std::vector<double>& B,
        const std::vector<double>& C,
        double D,
        double Ts,
        size_t bufferSize)
    : DiscreteSystem(Ts, bufferSize),
      A_(A),
      B_(B),
      C_(C),
      D_(D)
{
    // ---- Validación de dimensiones ----
    if (A.empty())
        throw std::runtime_error("InvalidDimensions: A must not be empty");

    size_t n = A.size();

    for (const auto& row : A) {
        if (row.size() != n)
            throw std::runtime_error("InvalidDimensions: A must be square (n×n)");
    }

    if (B.size() != n)
        throw std::runtime_error("InvalidDimensions: B must have size n");

    if (C.size() != n)
        throw std::runtime_error("InvalidDimensions: C must have size n");

    // Guardamos n
    n_ = n;

    // Inicializamos el estado x(k)
    x_.assign(n_, 0.0);

    std::cout << "StateSpaceSystem creado correctamente (n = " << n_ << ")" << std::endl;
}


/**
 * @brief Calcula la salida y actualiza el estado del sistema
 * 
 * Implementa las ecuaciones:
 *   y(k) = C·x(k) + D·u(k)
 *   x(k+1) = A·x(k) + B·u(k)
 */
double StateSpaceSystem::compute(double uk)
{
    // ---- 1. Salida y(k) = C*x + D*u ----
    double yk = 0.0;

    for (size_t i = 0; i < n_; i++)
        yk += C_[i] * x_[i];

    yk += D_ * uk;

    // ---- 2. Actualizar estado x(k+1) = A*x + B*u ----
    std::vector<double> x_next(n_, 0.0);

    for (size_t i = 0; i < n_; i++) {       // fila de A
        for (size_t j = 0; j < n_; j++) {   // columna de A
            x_next[i] += A_[i][j] * x_[j];
        }
        x_next[i] += B_[i] * uk;
    }

    // Copiar x(k+1)
    x_ = x_next;

    return yk;
}


/**
 * @brief Reinicia el estado interno del sistema
 * 
 * Establece el vector de estado x(k) = 0, preparando el sistema
 * para una nueva simulación desde condiciones iniciales.
 */
void StateSpaceSystem::resetState()
{
    std::fill(x_.begin(), x_.end(), 0.0);
}



/**
 * @brief Sobrecarga del operador << para imprimir el sistema
 * 
 * Muestra las matrices A, B, C, D y el vector de estado x de forma
 * legible con formato alineado.
 */
std::ostream& operator<<(std::ostream& os, const StateSpaceSystem& sys)
{
    os << "StateSpaceSystem (n = " << sys.getState().size() << ")\n";

    os << "A = [\n";
    for (const auto& row : sys.getA()) {
        os << "  ";
        for (double v : row)
            os << std::setw(10) << v << " ";
        os << "\n";
    }
    os << "]\n";

    os << "B = [ ";
    for (double v : sys.getB()) os << v << " ";
    os << "]\n";

    os << "C = [ ";
    for (double v : sys.getC()) os << v << " ";
    os << "]\n";

    os << "D = " << sys.getD() << "\n";

    os << "x = [ ";
    for (double v : sys.getState()) os << v << " ";
    os << "]\n";

    return os;
}

} // namespace DiscreteSystems
