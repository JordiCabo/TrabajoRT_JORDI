/**
 * @file Sumador.cpp
 * @brief Implementación del sumador restador para cálculo de error
 * @author Jordi + GitHub Copilot
 * @date 2025-12-18
 */

#include "Sumador.h"
#include <stdexcept>

namespace DiscreteSystems {

/**
 * @brief Constructor del sumador
 * 
 * Inicializa el sistema discreto base y establece e_out_ = 0 para
 * simular un error inicial nulo.
 */
Sumador::Sumador(double Ts, size_t bufferSize)
    : DiscreteSystem(Ts, bufferSize), e_out_(0.0) {}

/**
 * @brief Calcula el error como diferencia entre referencia y realimentación
 * Devuelve e(k) = ref - y.
 */
double Sumador::compute(double ref, double y) {
    e_out_ = ref - y;
    return e_out_;
}

/**
 * @brief Método compute() de una sola entrada (sobrecarga heredada)
 * @throws std::runtime_error indicando que debe usarse compute(ref, y)
 * 
 * Esta función existe por herencia, pero no es válida para Sumador (requiere 2 entradas).
 */
double Sumador::compute(double) {
    throw std::runtime_error(
        "Sumador necesita 2 entradas: use compute(ref, y)");
}

/**
 * @brief Reinicia el estado interno del sumador
 * 
 * Limpia e_out_ = 0 para preparar el sumador para una nueva
 * simulación desde condiciones iniciales.
 */
void Sumador::resetState() {
    e_out_ = 0.0;
}

} // namespace DiscreteSystems
