#pragma once
#include <mutex>

class VariablesCompartidas {
public:
    VariablesCompartidas();   // Constructor

    // Variables compartidas
    double ref;     // Referencia del sistema
    double e;       // Error: ref - ykd
    double u;       // Salida del PID (digital)
    double ua;      // Acción de control analógica tras DA
    double yk;      // Salida de la planta (analógica)
    double ykd;     // Salida digital tras AD
    bool running;  // Indicador de ejecución del hilo

    // Mutex para proteger todas las variables
    std::mutex mtx;
};