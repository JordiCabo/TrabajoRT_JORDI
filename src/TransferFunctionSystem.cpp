
/**
 * @file TransferFunctionSystem.cpp
 * @brief Implementación del sistema discreto basado en función de transferencia
 * @author Jordi + GitHub Copilot
 * @date 2025-12-18
 */

#include "TransferFunctionSystem.h"

namespace DiscreteSystems {

/**
 * @brief Constructor del sistema con función de transferencia
 * 
 * Normaliza automáticamente los coeficientes del denominador para que a[0] = 1,
 * e inicializa los historiadores de entrada y salida con ceros.
 */
TransferFunctionSystem::TransferFunctionSystem(const std::vector<double>& b,
                                               const std::vector<double>& a,
                                               double Ts,
                                               size_t bufferSize)
    : DiscreteSystem(Ts, bufferSize), b_(b), a_(a)
{
    // Normalizar a[0] = 1 si es distinto de 1
    if (!a_.empty() && a_[0] != 1.0) {
        for (auto &ai : a_) ai /= a_[0];
    }

    // Inicializar historiadores con ceros
    uHist_ = std::vector<double>(b_.size(), 0.0);       // tamaño m+1
    yHist_ = std::vector<double>(a_.size() - 1, 0.0);   // tamaño n

    std::cout << "Objeto de tipo TransferFunctionSystem creado correctamente" << std::endl;
}

/**
 * @brief Calcula la salida del sistema mediante ecuación en diferencias
 * 
 * Implementa la ecuación en diferencias:
 *   y(k) = [b[0]*u(k) + b[1]*u(k-1) + ... + b[m]*u(k-m)
 *           - a[1]*y(k-1) - a[2]*y(k-2) - ... - a[n]*y(k-n)] / a[0]
 * 
 * Nota: a[0] = 1 siempre por normalización realizada en el constructor.
 */
double TransferFunctionSystem::compute(double uk){
       
    // --- Desplazar historial de entradas ---
    for (size_t i = uHist_.size() - 1; i > 0; --i)
        uHist_[i] = uHist_[i - 1];
    uHist_[0] = uk;

    // --- Cálculo de la parte del numerador (contribución de entradas) ---
    double yk = 0.0;
    for (size_t i = 0; i < b_.size(); ++i)
        yk += b_[i] * uHist_[i];

    // --- Cálculo de la parte del denominador (retroalimentación de salidas) ---
    // Nota: a_[0] = 1 siempre (por normalización)
    for (size_t i = 1; i < a_.size(); ++i)
        yk -= a_[i] * yHist_[i - 1];

    // --- Normalización por a[0] (siempre vale 1, pero por claridad se mantiene) ---
    yk /= a_[0];

    // --- Actualizar historial de salidas ---
    for (size_t i = yHist_.size() - 1; i > 0; --i)
        yHist_[i] = yHist_[i - 1];
    yHist_[0] = yk;

    return yk;
}

/**
 * @brief Reinicia el estado interno del sistema
 * 
 * Limpia los historiadores de entrada y salida, preparando el sistema
 * para una nueva simulación desde condiciones iniciales.
 */
void TransferFunctionSystem::resetState(){
    std::cout << "ResetState ejecutado" << std::endl;
}

} // namespace DiscreteSystems


