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
 * #include "system_config.h"
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

} // namespace SystemConfig

#endif // SYSTEM_CONFIG_H
