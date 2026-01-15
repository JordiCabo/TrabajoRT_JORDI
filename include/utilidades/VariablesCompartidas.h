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
#include <iostream>
#include <vector>
#include <string>

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
    
    /**
     * @brief Sobrecarga del operador << para impresión formateada de variables del sistema
     * 
     * Permite usar `std::cout << vars` para imprimir el estado del lazo:
     * k=N | Ref=X | e=X | u=X | yk=X | ykd=X
     * 
     * @param os Stream de salida
     * @param vars Variables compartidas a imprimir
     * @return Referencia al stream para encadenamiento
     */
    friend std::ostream& operator<<(std::ostream& os, const VariablesCompartidas& vars);
    
    // ========================================
    // Buffer para grabación en fichero
    // ========================================
    
    /**
     * @brief Guarda una muestra en el buffer de memoria (operación O(1), real-time safe)
     * @param k Número de iteración
     * @param linea Texto de la línea a guardar
     * @note Llamar durante ejecución en tiempo real (no bloquea)
     */
    void guardarMuestraEnBuffer(int k, const std::string& linea);
    
    /**
     * @brief Escribe todo el buffer a disco (llamar DESPUÉS de terminar, no en tiempo real)
     * @param filename Ruta del fichero donde guardar
     * @return true si se escribió correctamente
     */
    bool escribirBufferADisco(const std::string& filename);
    
    /**
     * @brief Limpia el buffer de muestras
     */
    void limpiarBuffer();
    
    /**
     * @brief Retorna el número de muestras guardadas en buffer
     */
    size_t obtenerTamanoBuffer() const;
    
private:
    /// Buffer para almacenar las muestras (máximo 1000)
    std::vector<std::string> buffer_muestras;
    static constexpr size_t MAX_MUESTRAS = 1000;
};