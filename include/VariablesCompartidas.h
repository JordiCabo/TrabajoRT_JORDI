#pragma once
#include <pthread.h>

class VariablesCompartidas {
public:
    VariablesCompartidas();   // Constructor
    ~VariablesCompartidas();  // Destructor

    // Variables compartidas
    double ref;     // Referencia del sistema
    double e;       // Error: ref - ykd
    double u;       // Salida del PID (digital)
    double ua;      // Acción de control analógica tras DA
    double yk;      // Salida de la planta (analógica)
    double ykd;     // Salida digital tras AD
    bool running;  // Indicador de ejecución del hilo

    // Mutex POSIX para proteger todas las variables
    pthread_mutex_t mtx;
};