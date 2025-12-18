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
 * 
 * @param ref Valor de referencia deseada
 * @param y Valor medido real del sistema
 * @return Error e(k) = ref - y
 * 
 * Esta es la función correcta a utilizar; calcula directamente la diferencia
 * entre la referencia y la realimentación.
 */
double Sumador::compute(double ref, double y) {
    e_out_ = ref - y;
    return e_out_;
}

/**
 * @brief Método compute() de una sola entrada (sobrecarga heredada)
 * 
 * @param uk Entrada única (no utilizada)
 * @return Nunca retorna
 * @throws std::runtime_error indicando que debe usarse compute(ref, y)
 * 
 * Esta función se implementa por la herencia de DiscreteSystem (virtual puro),
 * pero no es válida para el Sumador que requiere dos entradas independientes.
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
