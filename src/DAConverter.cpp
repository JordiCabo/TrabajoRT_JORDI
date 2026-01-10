/**
 * @file DAConverter.cpp
 * @brief Implementación del convertidor Digital-Analógico
 * @author Jordi + GitHub Copilot
 * @date 2025-12-18
 */

#include "DAConverter.h"

namespace DiscreteSystems {

/**
 * @brief Constructor del convertidor D/A con retenedor ZOH
 * 
 * Inicializa el sistema discreto base y establece u_out_ = 0 para
 * simular una salida inicial nula.
 */
DAConverter::DAConverter(double Ts, size_t bufferSize)
    : DiscreteSystem(Ts, bufferSize), u_out_(0.0)
{
    std::cout << "Objeto de tipo DAConverter creado correctamente" << std::endl;
}

/**
 * @brief Calcula la salida del convertidor D/A mediante retenedor de orden cero (ZOH)
 * Implementa un retenedor ZOH ideal: la salida es igual a la entrada y se
 * mantiene constante durante el período de muestreo.
 */
double DAConverter::compute(double uk)
{
    // Salida instantánea (ZOH): sin retardo
    u_out_ = uk;
    return u_out_;
}

/**
 * @brief Reinicia el estado interno del convertidor D/A
 * 
 * Limpia u_out_ = 0 para preparar el sistema para una nueva
 * simulación desde condiciones iniciales.
 */
void DAConverter::resetState()
{
    u_out_ = 0.0;
    std::cout << "ResetState de DAConverter ejecutado" << std::endl;
}

} // namespace DiscreteSystems
