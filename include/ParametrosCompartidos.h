/**
 * @file ParametrosCompartidos.h
 * @brief Gestión thread-safe de parámetros PID compartidos entre procesos
 * @author Jordi + GitHub Copilot
 * @date 2026-01-03
 * 
 * Proporciona una clase para almacenar y sincronizar parámetros del controlador PID
 * que deben ser compartidos entre múltiples hilos o procesos.
 */

#pragma once
#include <pthread.h>

/**
 * @class ParametrosCompartidos
 * @brief Gestión thread-safe de parámetros dinámicos del controlador PID
 * 
 * Almacena los parámetros Kp, Ki, Kd, setpoint y tipo de señal de referencia
 * con acceso sincronizado mediante mutex POSIX. Permite que el proceso GUI
 * y el proceso simulador actualicen los parámetros sin condiciones de carrera.
 * 
 * Patrón de uso (en HiloPID):
 * @code{.cpp}
 * {
 *     std::lock_guard<pthread_mutex_t> lock(params->mtx);
 *     current_kp = params->kp;
 *     current_ki = params->ki;
 *     current_kd = params->kd;
 * }
 * // Usar current_kp, current_ki, current_kd para calcular salida
 * @endcode
 * 
 * @invariant mtx es un mutex POSIX válido después de construcción
 * @invariant Acceso a miembros requiere lock(mtx) para thread-safety
 */
class ParametrosCompartidos {
public:
	/**
	 * @brief Constructor. Inicializa el mutex POSIX y establece valores por defecto.
	 * 
	 * Valores iniciales:
	 * - kp = 1.0, ki = 0.5, kd = 0.1
	 * - setpoint = 0.0
	 * - signal_type = 1 (escalón)
	 */
	ParametrosCompartidos();

	/**
	 * @brief Destructor. Destruye el mutex POSIX.
	 */
	~ParametrosCompartidos();

	// ========================================
	// Parámetros compartidos del PID
	// ========================================
	
	double kp;			///< Ganancia proporcional (sintonizable en línea)
	double ki;			///< Ganancia integral (sintonizable en línea)
	double kd;			///< Ganancia derivativa (sintonizable en línea)
	double setpoint;	///< Referencia deseada del sistema (setpoint)
	int signal_type;	///< Tipo de señal de referencia (1=escalón, 2=rampa, 3=senoidal, 4=PWM)

	// ========================================
	// Sincronización
	// ========================================
	
	/// Mutex POSIX que protege acceso a kp, ki, kd, setpoint, signal_type
	/// @warning CRÍTICO: Siempre usar lock_guard o pthread_mutex_lock antes de acceder a miembros
	pthread_mutex_t mtx;
};
