#include <iostream>
#include "DAConverter.h"

int main() {
    using namespace DiscreteSystems;

    // --- Crear DAConverter ---
    double Ts = 0.1;  // período de muestreo
    DAConverter dac(Ts);

    // --- Parámetros de simulación ---
    const int N = 20;
    double input = 0.0;

    std::cout << "Paso\tEntrada\tSalida" << std::endl;

    for (int k = 0; k < N; ++k) {
        // --- Generar señal escalón a partir del paso 5 ---
        if (k >= 5) input = 1.0;

        // --- Llamar a next() ---
        double salida = dac.next(input);

        // --- Mostrar resultados ---
        std::cout << k << "\t" << input << "\t" << salida << std::endl;
    }

    return 0;
}
