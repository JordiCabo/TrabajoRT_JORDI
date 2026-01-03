#pragma once
#include <pthread.h>

/**
 * @class ParametrosCompartidos
 * @brief Clase para gestionar parámetros compartidos entre el regulador PID y la aplicación.
 * 
 * Proporciona acceso thread-safe a los parámetros del controlador PID (Kp, Ki, Kd)
 * mediante mutex POSIX, permitiendo que la GUI y el simulador actualicen los valores
 * de forma segura sin carreras de datos.
 */
class ParametrosCompartidos {
public:
	/**
	 * @brief Constructor. Inicializa el mutex y establece valores por defecto.
	 */
	ParametrosCompartidos();

	/**
	 * @brief Destructor. Destruye el mutex POSIX.
	 */
	~ParametrosCompartidos();

	// Parámetros compartidos del controlador PID
	double kp;			///< Ganancia proporcional
	double ki;			///< Ganancia integral
	double kd;			///< Ganancia derivativa
	double setpoint;	///< Referencia deseada del sistema
	int signal_type;	///< Tipo de señal (1=step, 2=sine, 3=pwm)

	// Mutex POSIX para proteger acceso a los parámetros
	pthread_mutex_t mtx;
};
