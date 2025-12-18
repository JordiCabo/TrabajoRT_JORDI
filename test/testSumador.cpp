#include <iostream>
#include "Sumador.h"

using namespace DiscreteSystems;

int main() {
    // Crear sumador con Ts = 0.01 y buffer size por defecto
    double Ts = 0.01;
    Sumador sumador(Ts);

    // Valores para probar
    double referencias[] = {1.0, 1.0, 0.5, 0.0, -1.0};
    double salidas[]     = {0.2, 0.9, 1.0, -0.1, -1.5};

    int N = sizeof(referencias) / sizeof(referencias[0]);

    std::cout << "=== Test del Sumador ===\n";

    for (int k = 0; k < N; ++k) {
        double ref = referencias[k];
        double y     = salidas[k];

        double e = sumador.compute(ref, y);

        std::cout << "k=" << k
                  << " | ref=" << ref
                  << " | y=" << y
                  << " | error=" << e
                  << std::endl;
    }

    // Comprobación rápida del último valor usando getLastOutput()
    std::cout << "\nÚltimo error almacenado = "
              << sumador.getLastOutput()
              << std::endl;

    return 0;
}
