/**
 * @file SignalSwitch.cpp
 * @brief Implementación del multiplexor de señales
 * @author Jordi + GitHub Copilot
 * @date 2026-01-03
 */

#include "../include/SignalSwitch.h"
#include <stdexcept>
#include <iostream>

namespace SignalGenerator {

SignalSwitch::SignalSwitch(std::shared_ptr<StepSignal> stepSignal,
                           std::shared_ptr<PwmSignal> pwmSignal,
                           std::shared_ptr<SineSignal> sineSignal,
                           int initialSelector)
    : stepSignal_(stepSignal)
    , pwmSignal_(pwmSignal)
    , sineSignal_(sineSignal)
    , selector_(initialSelector)
{
    // Validar que las señales no sean nullptr
    if (!stepSignal_ || !sineSignal_ || !pwmSignal_) {
        throw std::invalid_argument("SignalSwitch: Las señales no pueden ser nullptr");
    }

    // Validar selector inicial
    if (selector_ < 0 || selector_ > 2) {
        throw std::invalid_argument("SignalSwitch: El selector debe estar en rango [0,2]");
    }

    std::cout << "SignalSwitch creado con selector=" << selector_ << std::endl;
}

void SignalSwitch::setSelector(int selector) {
    if (selector < 0 || selector > 2) {
        throw std::invalid_argument("SignalSwitch::setSelector: El valor debe estar en rango [0,2]");
    }
    selector_ = selector;
}

double SignalSwitch::next() {
    // Leer selector y ejecutar next() de la señal correspondiente
    switch (selector_) {
        case 0:
            return stepSignal_->next();
        case 1:
            return pwmSignal_->next();
        case 2:
            return sineSignal_->next();
        default:
            throw std::logic_error("SignalSwitch::next: Selector inválido");
    }
}

} // namespace SignalGenerator
