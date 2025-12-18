#include <iostream>
#include "PIDController.h"

int main() {
    using namespace DiscreteSystems;

    // --- Crear PID: Kp, Ki, Kd, Ts ---
    double Kp = 2.0;
    double Ki = 1.0;
    double Kd = 0.5;
    double Ts = 0.1;  // periodo de muestreo 0.1 s

    PIDController pid(Kp, Ki, Kd, Ts);

    // --- Parámetros de la simulación ---
    const int N = 50;       // número de pasos
    double referencia = 1.0; // escalón unitario
    double y = 0.0;         // valor medido inicial
    double error;

    std::cout << "Paso\tError\tSalida u[k]" << std::endl;

    for (int k = 0; k < N; ++k) {
        // --- calcular error ---
        error = referencia - y;

        // --- llamar a next() para obtener u[k] ---
        double u = pid.next(error);

        // --- simular respuesta del sistema (motor simple) ---
        // para test, hacemos y[k+1] = y[k] + Ts*u[k]
        y = y + Ts * u;

        // --- imprimir resultados ---
        std::cout << k << "\t" << error << "\t" << u << std::endl;
    }

    return 0;
}