/*
 * @file config.h
 * 
 * @author Corsino (definición de parámetros pedagógicos)
 * @author GitHub Copilot (organización)
 * 
 * @brief Parámetros de configuración del sistema de control PID
 * 
 * Define constantes y macros para:
 *  - Modo de representación: simple (3 gráficas) vs debug (6 gráficas)
 *  - Parámetros PID: rangos, valores iniciales de Kp, Ki, Kd
 *  - Setpoint: rango y valor inicial
 *  - Gráficas: número de puntos históricos a mostrar
 *  - Periodo de actualización: frecuencia de simulación
 * 
 * @note Modificar estos valores para:
 *       - Cambiar dinámicas del sistema
 *       - Ajustar rango de parámetros en GUI
 *       - Tuning de constantes PID
 *       - Cambiar entre modo pedagógico simple y debug avanzado
 */

#ifndef CONFIG_H
#define CONFIG_H

// =====================================================
// CONFIGURACIÓN DE INTERFAZ GRÁFICO
// =====================================================

// === MODO REPRESENTACIÓN ===
#define REPRESENTACION_SIMPLE   1  // 1=Simple (3 gráficas), 0=Debug (6 gráficas)

// === GRÁFICAS ===
#if REPRESENTACION_SIMPLE
    #define NUM_CHARTS          3
#else
    #define NUM_CHARTS          6
#endif

#define MAX_DATA_POINTS         500  // Puntos históricos por gráfica (configurable)
#define CHART_UPDATE_MS         100  // Frecuencia de actualización QLineSeries (ms)
                                      // IMPORTANTE: Sincronizar con período de muestreo del proceso externo

// === PARÁMETROS PID ===
#define KP_MIN              0.0
#define KP_MAX              10.0
#define KP_STEP             0.1
#define KP_DEFAULT          1.0

#define KI_MIN              0.0
#define KI_MAX              10.0
#define KI_STEP             0.1
#define KI_DEFAULT          0.5

#define KD_MIN              0.0
#define KD_MAX              10.0
#define KD_STEP             0.1
#define KD_DEFAULT          0.2

// === SETPOINT (soporta valores negativos) ===
#define SETPOINT_MIN        -50.0    // Ajustar según aplicación
#define SETPOINT_MAX        150.0    // Ajustar según aplicación
#define SETPOINT_STEP       0.1
#define SETPOINT_DEFAULT    0.0

// === RANGO DE VISUALIZACIÓN GRÁFICAS ===
#define VALUE_MIN           -10.0
#define VALUE_MAX           110.0

#endif // CONFIG_H
