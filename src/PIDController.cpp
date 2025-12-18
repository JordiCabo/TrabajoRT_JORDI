/**
 * @file PIDController.cpp
 * @brief Implementación del controlador PID discreto
 * @author Jordi + GitHub Copilot
 * @date 2025-12-18
 */

#include "PIDController.h"

namespace DiscreteSystems {

/**
 * @brief Constructor del controlador PID
 * 
 * Inicializa los parámetros de ganancia (Kp, Ki, Kd) y prepara los
 * historiadores de error y control vacíos para la primera ejecución.
 */
PIDController::PIDController(double Kp, double Ki, double Kd, double Ts, 
                  size_t bufferSize):
    DiscreteSystem(Ts, bufferSize), Kp_(Kp), Ki_(Ki), Kd_(Kd)  
{
     eHist_.clear();
    uHist_.clear();
    std::cout << "Objeto de tipo PIDController creado correctamente" << std::endl;
}

/**
 * @brief Calcula la acción de control PID discreto mediante ecuación en diferencias
 * 
 * @param e_k Error en el paso k (e(k) = referencia - realimentación)
 * @return Acción de control u(k)
 * 
 * Implementa la ecuación en diferencias:
 *   Δu(k) = a₀·e(k) + a₁·e(k-1) + a₂·e(k-2)
 *   u(k) = u(k-1) + Δu(k)
 * 
 * Donde los coeficientes son:
 *   a₀ = Kp + Ki·Ts + Kd/Ts
 *   a₁ = -Kp - 2·Kd/Ts
 *   a₂ = Kd/Ts
 */
double PIDController::compute(double e_k){
       
     double e_k1 = (eHist_.size() >= 1) ? eHist_.back() : 0.0;          // e[k-1]
double e_k2 = (eHist_.size() >= 2) ? eHist_[eHist_.size() - 2] : 0.0; // e[k-2]
    double u_k_minus_1 = uHist_.empty() ? 0.0 : uHist_.back();

    double a0 = Kp_ + Ki_ * getSamplingTime() + Kd_ / getSamplingTime();
    double a1 = -Kp_ - 2.0 * Kd_ / getSamplingTime();
    double a2 = Kd_ / getSamplingTime();

    double delta_u = a0 * e_k + a1 * e_k1 + a2 * e_k2;
    double u_k = u_k_minus_1 + delta_u;

    eHist_.push_back(e_k);
    uHist_.push_back(u_k);

    return u_k;

   
}

/**
 * @brief Reinicia el estado interno del controlador PID
 * 
 * Limpia los historiadores de error y control para preparar el
 * PID para una nueva simulación desde condiciones iniciales.
 */
void PIDController::resetState(){
    eHist_.clear();
    uHist_.clear();
    std::cout << "ResetState ejecutado" << std::endl;
}

/**
 * @brief Sobrecarga del operador << para imprimir información del PID
 * 
 * @param os Stream de salida
 * @param pid Controlador PID a imprimir
 * @return Referencia al stream
 * 
 * Muestra la última acción de control calculada o un mensaje si aún
 * no hay salidas disponibles.
 */
std::ostream& operator<<(std::ostream& os, const PIDController& pid)
{
    if (!pid.uHist_.empty()) {
        os << "Última salida u[k] = " << pid.uHist_.back();
    } else {
        os << "No hay salidas aún";
    }
    return os;
}


} 

 