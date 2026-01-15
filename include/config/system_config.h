/**
 * @file system_config.h
 * @brief Configuración centralizada del simulador - Single Source of Truth (SSOT)
 * @author Jordi + GitHub Copilot
 * @date 2026-01-11
 * @version 1.0.6
 * 
 * Define todas las constantes de configuración del simulador en un único lugar:
 * - Períodos de muestreo y frecuencias de ejecución
 * - Tamaños de buffers y parámetros de logging
 * - Configuración de sincronización y timeouts
 * 
 * Uso:
 * @code{.cpp}
 * #include "config/system_config.h"
 * 
 * double Ts = SystemConfig::TS_CONTROLLER;  // 0.01 s = 10 ms
 * double freq = SystemConfig::FREQ_CONTROLLER;  // 100 Hz
 * size_t buffer = SystemConfig::BUFFER_SIZE;  // 1000 muestras
 * @endcode
 */

#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include <cstddef>

namespace SystemConfig {

//-------------------------------------------------------------
// Parámetros PID
//-------------------------------------------------------------
constexpr double PID_KP = 5.0;
constexpr double PID_KI = 3.0;
constexpr double PID_KD = 0.7;
constexpr double PID_SETPOINT = 1.0;

//-------------------------------------------------------------
// Parámetros Planta
//-------------------------------------------------------------
constexpr double PLANTA_TAU = 1.0;
constexpr double PLANTA_NUM_S[] = {1.0};
constexpr double PLANTA_DEN_S[] = {1.0, 1.0};

//-------------------------------------------------------------
// Parámetros Señales
//-------------------------------------------------------------
constexpr double SIGNAL_AMPLITUDE = 1.0;
constexpr double SIGNAL_STEP_TIME = 0.05;
constexpr double SIGNAL_OFFSET = 0.0;
constexpr double SIGNAL_SIN_FREQ = 1.0;
constexpr double SIGNAL_SIN_PHASE = 0.0;
constexpr double SIGNAL_SIN_AMP = 10.0;
constexpr double SIGNAL_PWM_DUTY = 0.5;
constexpr double SIGNAL_PWM_PERIOD = 1.0;
constexpr int SIGNAL_INITIAL_TYPE = 0; // 0=step, 1=pwm, 2=sine

//-------------------------------------------------------------
// Frecuencias y Períodos de Muestreo
//-------------------------------------------------------------

/// Período de muestreo del controlador PID (segundos)
constexpr double TS_CONTROLLER = 0.01;  // 10 ms = 100 Hz

/// Período de muestreo de componentes auxiliares (segundos)
constexpr double TS_COMPONENT = TS_CONTROLLER / 10.0;  // 1 ms = 1000 Hz

/// Frecuencia del controlador PID (Hz)
constexpr double FREQ_CONTROLLER = 1.0 / TS_CONTROLLER;  // 100 Hz

/// Frecuencia de componentes auxiliares (Hz)
constexpr double FREQ_COMPONENT = 1.0 / TS_COMPONENT;  // 1000 Hz

/// Frecuencia de comunicación IPC (GUI ↔ Simulador) (Hz)
constexpr double FREQ_COMMUNICATION = 10.0;  // 100 ms

//-------------------------------------------------------------
// Buffers y Logging
//-------------------------------------------------------------

/// Tamaño del buffer circular de muestras en DiscreteSystem
constexpr size_t BUFFER_SIZE_SAMPLES = 100;

/// Tamaño del buffer circular del RuntimeLogger (líneas)
constexpr size_t BUFFER_SIZE_LOGGER = 1000;

/// Intervalo de flush del RuntimeLogger (cada N líneas)
constexpr size_t LOGGER_FLUSH_INTERVAL = 100;

/// Directorio de logs
constexpr const char* LOG_DIRECTORY = "logs";

//-------------------------------------------------------------
// Timeouts y Sincronización
//-------------------------------------------------------------

/// Timeout de timedlock como fracción del período (0.2 = 20%)
constexpr double TIMEDLOCK_TIMEOUT_FRACTION = 0.2;

/// Umbral WARNING de uso del período (0.9 = 90%)
constexpr double WARNING_THRESHOLD = 0.9;

/// Umbral CRITICAL de uso del período (1.0 = 100%)
constexpr double CRITICAL_THRESHOLD = 1.0;

//-------------------------------------------------------------
// Nombres de hilos (para logging, identificación, etc.)
//-------------------------------------------------------------
constexpr const char* HILO_REF_NAME         = "hiloRef";
constexpr const char* HILO_PLANTA_NAME      = "hiloPlanta";
constexpr const char* HILO_AD_NAME          = "hiloAD";
constexpr const char* HILO_DA_NAME          = "hiloDA";
constexpr const char* HILO_PID_NAME         = "hiloPID";
constexpr const char* HILO_SUMADOR_NAME     = "Sumador";
constexpr const char* HILO_TRANSMISOR_NAME  = "hiloTransmisor";
constexpr const char* HILO_RECEPTOR_NAME    = "hiloReceptor";
constexpr const char* HILO_INTERRUPT_NAME   = "hiloInterruptor";

} // namespace SystemConfig

#endif // SYSTEM_CONFIG_H
