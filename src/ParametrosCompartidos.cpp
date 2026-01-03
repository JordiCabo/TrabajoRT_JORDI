#include "../include/ParametrosCompartidos.h"

ParametrosCompartidos::ParametrosCompartidos() {
	// Inicializar parámetros con valores por defecto
	kp = 1.0;
	ki = 0.5;
	kd = 0.1;
	setpoint = 1.0;	signal_type = 0;  // Por defecto: escalón (0=step, 1=sine, 2=pwm)
	// Inicializar mutex POSIX con atributos por defecto
	pthread_mutex_init(&mtx, nullptr);
}

ParametrosCompartidos::~ParametrosCompartidos() {
	// Destruir el mutex POSIX
	pthread_mutex_destroy(&mtx);
}
