/**
 * @file ADConverter.cpp
 * @brief Implementación del convertidor Analógico-Digital
 * @author Jordi + GitHub Copilot
 * @date 2025-12-18
 */

#include "ADConverter.h"

namespace DiscreteSystems {

/**
 * @brief Constructor del convertidor A/D
 * 
 * Inicializa el sistema discreto base y establece u_prev_ = 0 para
 * simular una entrada previa nula al inicio de la simulación.
 */
ADConverter::ADConverter(double Ts, size_t bufferSize)
    : DiscreteSystem(Ts, bufferSize), u_prev_(0.0)
{
    std::cout << "Objeto de tipo ADConverter creado correctamente" << std::endl;
}

/**
 * @brief Calcula la salida del convertidor A/D mediante la ecuación y(k) = u(k-1)
 * 
 * @param uk Entrada muestreada en el paso k
 * @return Salida y(k) = u(k-1) (valor del paso anterior)
 * 
 * Implementa el retardo de un período de muestreo típico de un conversor A/D real.
 */
double ADConverter::compute(double uk)
{
     // La salida es el valor previo (retardo de 1 paso)
    double yk = u_prev_;

    // Actualizamos u_prev_ para el siguiente paso
    u_prev_ = uk;

    return yk;
}

/**
 * @brief Reinicia el estado interno del convertidor A/D
 * 
 * Limpia u_prev_ = 0 para preparar el sistema para una nueva
 * simulación desde condiciones iniciales.
 */
void ADConverter::resetState()
{
    u_prev_ = 0.0;
    std::cout << "ResetState de ADConverter ejecutado" << std::endl;
}

} // namespace DiscreteSystems
