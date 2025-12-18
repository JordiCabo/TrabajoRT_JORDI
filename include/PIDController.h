/**
 * @file PIDController.h
 * @brief Controlador PID discreto
 * @author Jordi + GitHub Copilot
 * @date 2025-12-18
 * 
 * Implementa un controlador PID discreto utilizando la ecuación en diferencias
 * con incrementos de control (velocidad de cambio).
 */

#ifndef DISCRETESYSTEMS_PIDCOTROLLER_H
#define DDISCRETESYSTEMS_PIDCOTROLLER_H

#include "DiscreteSystem.h"
#include <vector>

namespace DiscreteSystems {

/**
 * @class PIDController
 * @brief Controlador PID discreto con ecuación en diferencias
 * 
 * Implementa un controlador PID en tiempo discreto mediante la ecuación
 * en diferencias (versión de velocidad):
 * 
 *   Δu(k) = a₀·e(k) + a₁·e(k-1) + a₂·e(k-2)
 *   u(k) = u(k-1) + Δu(k)
 * 
 * Donde los coeficientes dependen de Kp, Ki, Kd y el período de muestreo Ts:
 *   a₀ = Kp + Ki·Ts + Kd/Ts
 *   a₁ = -Kp - 2·Kd/Ts
 *   a₂ = Kd/Ts
 * 
 * @invariant eHist_.size() <= 3 (almacena e(k), e(k-1), e(k-2))
 * @invariant uHist_.size() contiene el historial de salidas de control
 */
class PIDController : public DiscreteSystem {
public:
    /**
     * @brief Constructor del controlador PID
     * @param Kp Ganancia proporcional (debe ser > 0)
     * @param Ki Ganancia integral (puede ser >= 0)
     * @param Kd Ganancia derivativa (puede ser >= 0)
     * @param Ts Período de muestreo en segundos (debe ser > 0)
     * @param bufferSize Tamaño del buffer circular de muestras (por defecto 100)
     * @throws InvalidSamplingTime si Ts <= 0
     * 
     * @note Los historiadores de error y control se inicializan vacíos
     */
    PIDController(double Kp, double Ki, double Kd, double Ts, 
                  size_t bufferSize = 100);

    /**
     * @brief Obtiene la ganancia proporcional actual
     * @return Valor de Kp
     */
    double getKp() const;

    /**
     * @brief Obtiene la ganancia integral actual
     * @return Valor de Ki
     */
    double getKi() const;

    /**
     * @brief Obtiene la ganancia derivativa actual
     * @return Valor de Kd
     */
    double getKd() const;

    /**
     * @brief Obtiene la última acción de control calculada
     * @return Valor u[k-1]
     */
    double getLastControl() const;

    /**
     * @brief Sobrecarga del operador << para imprimir parámetros del PID
     * @param os Stream de salida
     * @param pid Controlador PID a imprimir
     * @return Referencia al stream
     */
    friend std::ostream& operator<<(std::ostream& os, const PIDController& pid);
    
    /**
     * @brief Actualiza la ganancia proporcional durante la ejecución
     * @param Kp Nuevo valor de ganancia proporcional
     * 
     * Permite sintonización en línea del parámetro Kp.
     */
    void setKp(double Kp);

    /**
     * @brief Actualiza la ganancia integral durante la ejecución
     * @param Ki Nuevo valor de ganancia integral
     * 
     * Permite sintonización en línea del parámetro Ki.
     */
    void setKi(double Ki);

    /**
     * @brief Actualiza la ganancia derivativa durante la ejecución
     * @param Kd Nuevo valor de ganancia derivativa
     * 
     * Permite sintonización en línea del parámetro Kd.
     */
    void setKd(double Kd);

    /**
     * @brief Actualiza todas las ganancias PID simultáneamente
     * @param Kp Nueva ganancia proporcional
     * @param Ki Nueva ganancia integral
     * @param Kd Nueva ganancia derivativa
     * 
     * Más eficiente que llamar a setKp(), setKi() y setKd() por separado.
     */
    void setGains(double Kp, double Ki, double Kd);

    /**
     * @brief Calcula la acción de control basada en el error
     * 
     * Implementa la ecuación en diferencias PID discreto con historia
     * de errores.
     * 
     * @param ek Error en el paso k (e(k) = referencia - realimentación)
     * @return Acción de control u(k) en el paso k
     * 
     * @invariant Después de ejecutar, eHist_ contiene los últimos 3 errores
     */
    double compute(double ek) override;

    /**
     * @brief Reinicia el estado interno del controlador
     * 
     * Limpia los historiadores de error y control, preparando el
     * PID para una nueva simulación desde condiciones iniciales.
     */
    void resetState() override;

private:
    std::vector<double> uHist_;   ///< Historial de acciones de control [u(k), u(k-1), ...]
    std::vector<double> eHist_;   ///< Historial de errores [e(k), e(k-1), e(k-2)]
    double Kp_;                   ///< Ganancia proporcional
    double Ki_;                   ///< Ganancia integral
    double Kd_;                   ///< Ganancia derivativa
};

} // namespace DiscreteSystems
#endif //DISCRETESYSTEMS_PIDCOTROLLER_H