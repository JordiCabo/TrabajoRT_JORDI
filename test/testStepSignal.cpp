#include <iostream>
#include "SignalGenerator.h"

int main() {
    using namespace SignalGenerator;

    // Parámetros del escalón
    double Ts = 0.01;       // período de muestreo [s]
    double amplitude = 1.0; // amplitud del escalón
    double step_time = 0.05; // tiempo donde se produce el escalón
    double offset = 0.0;     // desplazamiento vertical
    size_t N = 20;           // número de muestras a generar

    // Crear la señal escalón
    StepSignal step(Ts, amplitude, step_time, offset);

    std::cout << "t[s]\tvalue\n";
    for (size_t k = 0; k < N; ++k) {
        double value = step.next(); // genera la siguiente muestra y avanza el tiempo
        std::cout << k*Ts << "\t" << value << "\n";
    }

    return 0;
}