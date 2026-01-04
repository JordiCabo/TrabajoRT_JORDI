/**
 * @file SignalSwitch.h
 * @brief Multiplexor de señales discretas
 * @author Jordi + GitHub Copilot
 * @date 2026-01-03
 * 
 * Proporciona un selector dinámico entre múltiples señales (StepSignal,
 * SineSignal, PwmSignal) mediante un índice de selección.
 */

#pragma once
#include "SignalGenerator.h"
#include <memory>

namespace SignalGenerator {

/**
 * @class SignalSwitch
 * @brief Multiplexor que selecciona entre 3 señales según un índice
 * 
 * Mantiene punteros a tres señales (escalón, seno, PWM) y ejecuta
 * el método next() de la señal seleccionada según un selector entero.
 * No es una Signal en sí misma, solo delega a las señales que contiene.
 * 
 * Patrón de uso:
 * @code{.cpp}
 * auto step = std::make_shared<StepSignal>(0.01, 10.0, 0.5);
 * auto sine = std::make_shared<SineSignal>(0.01, 5.0, 1.0);
 * auto pwm = std::make_shared<PwmSignal>(0.01, 8.0, 0.5, 1.0);
 * 
 * SignalSwitch sw(step, sine, pwm);
 * sw.setSelector(1);  // Selecciona step
 * double val = sw.next();  // Ejecuta step->next()
 * 
 * sw.setSelector(2);  // Selecciona sine
 * val = sw.next();  // Ejecuta sine->next()
 * @endcode
 * 
 * @invariant selector_ debe estar en rango [0, 2]
 * @invariant Las señales no deben ser nullptr
 */
class SignalSwitch {
public:
    /**
     * @brief Constructor del multiplexor de señales
     * 
     * @param stepSignal Puntero a señal de escalón
     * @param pwmSignal Puntero a señal PWM
     * @param sineSignal Puntero a señal senoidal
     * @param initialSelector Selector inicial (0=step, 1=pwm, 2=sine)
     * 
     * @throw std::invalid_argument si alguna señal es nullptr
     * @throw std::invalid_argument si initialSelector no está en [1,3]
     */
    SignalSwitch(std::shared_ptr<StepSignal> stepSignal,
                 std::shared_ptr<PwmSignal> pwmSignal,
                 std::shared_ptr<SineSignal> sineSignal,
                 int initialSelector = 0);

    /**
     * @brief Actualiza el selector de señal
     * 
     * @param selector Índice de selección (0=step, 1=pwm, 2=sine)
     * @throw std::invalid_argument si selector no está en [0,2]
     */
    void setSelector(int selector);

    /**
     * @brief Obtiene el selector actual
     * @return Índice de la señal seleccionada (0, 1 o 2)
     */
    int getSelector() const { return selector_; }

    /**
     * @brief Obtiene puntero a la señal de escalón
     * @return Puntero compartido a StepSignal
     */
    std::shared_ptr<StepSignal> getStepSignal() { return stepSignal_; }

    /**
     * @brief Obtiene puntero a la señal senoidal
     * @return Puntero compartido a SineSignal
     */
    std::shared_ptr<SineSignal> getSineSignal() { return sineSignal_; }

    /**
     * @brief Obtiene puntero a la señal PWM
     * @return Puntero compartido a PwmSignal
     */
    std::shared_ptr<PwmSignal> getPwmSignal() { return pwmSignal_; }

    /**
     * @brief Ejecuta next() en la señal seleccionada
     * 
     * Lee el selector y delega al método next() de la señal correspondiente:
     * - selector_ == 0 → stepSignal_->next()
     * - selector_ == 1 → pwmSignal_->next()
     * - selector_ == 2 → sineSignal_->next()
     * 
     * @return Valor de la muestra actual de la señal seleccionada
     */
    double next();

private:
    std::shared_ptr<StepSignal> stepSignal_;   ///< Señal de escalón
    std::shared_ptr<SineSignal> sineSignal_;   ///< Señal senoidal
    std::shared_ptr<PwmSignal> pwmSignal_;     ///< Señal PWM
    int selector_;                              ///< Selector actual (1, 2 o 3)
};

} // namespace SignalGenerator
