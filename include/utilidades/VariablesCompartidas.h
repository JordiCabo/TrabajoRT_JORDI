/**
 * @file VariablesCompartidas.h
 * @brief Gestión thread-safe de variables del lazo de control
 * @author Jordi + GitHub Copilot
 * @date 2026-01-03
 * 
 * Centraliza todas las variables compartidas del lazo de control:
 * referencia, error, acciones de control, realimentación, etc.
 */

#pragma once
#include <pthread.h>

/**
 * @class VariablesCompartidas
 * @brief Almacenamiento centralizado de variables del lazo de control con protección mutex
 * 
 * Gestiona el estado compartido entre múltiples hilos que ejecutan:
 * - Generador de señal de referencia (escribe ref)
 * - Sumador (lee ref, ykd; escribe e)
 * - Controlador PID (lee e; escribe u)
 * - Conversor D/A (lee u; escribe ua)
 * - Planta (lee ua; escribe yk)
 * - Conversor A/D (lee yk; escribe ykd)
 * 
 * Todos los accesos deben estar protegidos por mtx.
 * 
 * Estructura de datos (lazo de control):
 * @verbatim
 *   ref (referencia)
 *       ↓
 *   ┌─ Sumador (ref - ykd) → error (e)
 *   │
 *   └─ PID (e) → u (digital)
 *       ↓
 *   D/A Converter (u) → ua (analógica)
 *       ↓
 *   Planta (ua) → yk (analógica)
 *       ↓
 *   A/D Converter (yk) → ykd (digital)
 * @endverbatim
 * 
 * @invariant mtx es un mutex POSIX válido después de construcción
 * @invariant Acceso a cualquier miembro requiere lock(mtx) para thread-safety
 */
class VariablesCompartidas {
public:
    /**
     * @brief Constructor. Inicializa el mutex POSIX y establece variables a 0.0.
     */
    VariablesCompartidas();

    /**
     * @brief Destructor. Destruye el mutex POSIX.
     */
    ~VariablesCompartidas();

    // ========================================
    // Variables del lazo de control
    // ========================================
    
    double ref;     ///< Referencia deseada (referencia del sistema)
    double e;       ///< Error: e(k) = ref - ykd (entrada al controlador)
    double u;       ///< Salida del PID (u(k) en valores digitales)
    double ua;      ///< Acción de control analógica tras conversor D/A
    double yk;      ///< Salida de la planta (valor analógico observado)
    double ykd;     ///< Salida de la planta digitalizada tras conversor A/D
    bool running;   ///< Indicador de ejecución del lazo (true=ejecutando, false=detener)

    // ========================================
    // Sincronización
    // ========================================
    
    /// Mutex POSIX que protege acceso a ref, e, u, ua, yk, ykd, running
    /// @warning CRÍTICO: Siempre usar lock_guard o pthread_mutex_lock antes de acceder a variables
    /// 
    /// Patrón seguro:
    /// @code{.cpp}
    /// {
    ///     std::lock_guard<pthread_mutex_t> lock(vars.mtx);
    ///     double current_error = vars.e;
    ///     vars.u = pid_output;
    /// }
    /// @endcode
    pthread_mutex_t mtx;
};