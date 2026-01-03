#include "VariablesCompartidas.h"

VariablesCompartidas::VariablesCompartidas()
    : ref(0.0), e(0.0), u(0.0), ua(0.0), yk(0.0), ykd(0.0), running(false)
{
    // Inicializar mutex POSIX
    pthread_mutex_init(&mtx, nullptr);
}

VariablesCompartidas::~VariablesCompartidas()
{
    // Destruir mutex POSIX
    pthread_mutex_destroy(&mtx);
}
