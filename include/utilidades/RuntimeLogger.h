/**
 * @file RuntimeLogger.h
 * @brief Sistema de logging genérico con buffer circular para métricas de tiempo real
 * @author Jordi + GitHub Copilot
 * @date 2026-01-11
 * @version 1.0.7
 * 
 * Proporciona logging de métricas de runtime con:
 * - Buffer circular de N líneas (por defecto 1000)
 * - Escritura periódica a disco
 * - Generación automática de archivos con timestamp
 * - Headers personalizables
 */

#pragma once
#include <string>
#include <deque>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <sys/stat.h>

namespace DiscreteSystems {

/**
 * @class RuntimeLogger
 * @brief Logger genérico con buffer circular para métricas de tiempo real
 * 
 * Mantiene un buffer circular en memoria con las últimas N líneas de log.
 * Escribe periódicamente a disco para evitar I/O excesivo.
 * 
 * Patrón de uso:
 * @code{.cpp}
 * RuntimeLogger logger("HiloPID", 1000);  // Buffer de 1000 líneas
 * 
 * // Configurar header
 * logger.setHeader("Frequency: 100 Hz\nPeriod: 10000 us");
 * logger.setColumns({"Iteration", "t_espera_us", "t_ejec_us", "Status"});
 * 
 * // Escribir datos
 * std::ostringstream line;
 * line << std::setw(10) << iteration << std::setw(14) << tiempo;
 * logger.writeLine(line.str());
 * 
 * // Forzar escritura a disco
 * logger.flush();
 * @endcode
 */
class RuntimeLogger {
public:
    /**
     * @brief Constructor que crea el archivo de log con timestamp
     * 
     * @param prefix Prefijo del archivo (ej: "HiloPID", "HiloPlanta")
     * @param max_lines Máximo de líneas en buffer circular (default: 1000)
     * @param log_dir Directorio donde guardar logs (default: "../logs")
     */
    RuntimeLogger(const std::string& prefix, int max_lines = 1000, 
                  const std::string& log_dir = "../logs");
    
    /**
     * @brief Destructor que escribe el buffer final a disco
     */
    ~RuntimeLogger();
    
    /**
     * @brief Establece el header informativo del log
     * 
     * @param header Texto del header (puede contener \n para múltiples líneas)
     */
    void setHeader(const std::string& header);
    
    /**
     * @brief Establece los nombres de las columnas
     * 
     * @param columns Vector con nombres de columnas
     * @param widths Vector con anchos de columnas (opcional)
     */
    void setColumns(const std::vector<std::string>& columns, 
                    const std::vector<int>& widths = {});
    
    /**
     * @brief Añade una línea al buffer circular
     * 
     * Si el buffer está lleno, elimina la línea más antigua.
     * Escribe a disco cada N líneas (configurable con setFlushInterval).
     * 
     * @param line Línea formateada a añadir (debe incluir \n al final)
     * @param force_flush Si es true, escribe inmediatamente a disco
     */
    void writeLine(const std::string& line, bool force_flush = false);
    
    /**
     * @brief Fuerza escritura del buffer completo a disco
     */
    void flush();
    
    /**
     * @brief Configura intervalo de escritura automática
     * 
     * @param interval Número de líneas entre escrituras (0 = desactivar auto-flush)
     */
    void setFlushInterval(int interval);
    
    /**
     * @brief Obtiene la ruta del archivo de log
     */
    std::string getLogPath() const { return logfile_path_; }
    
    // ===== Métodos de inicialización predefinidos para hilos específicos =====
    
    /**
     * @brief Inicializa RuntimeLogger para HiloPID con columnas y header específico
     * 
     * @param frequency Frecuencia de ejecución en Hz
     */
    void initializeHiloPID(double frequency);
    
    /**
     * @brief Inicializa RuntimeLogger para Hilo con columnas y header específico
     * 
     * @param frequency Frecuencia de ejecución en Hz
     */
    void initializeHilo(double frequency);    
    /**
     * @brief Escribe una línea de timing (versión sobrecargada)
     * 
     * Formatea automáticamente los valores de timing y escribe al buffer.
     * 
     * @param iteration Número de iteración
     * @param t_espera_us Tiempo de espera del mutex (microsegundos)
     * @param t_ejec_us Tiempo de ejecución de la tarea (microsegundos)
     * @param t_total_us Tiempo total del ciclo (microsegundos)
     * @param periodo_us Período de muestreo configurado (microsegundos)
     * @param ts_real_us Período real medido entre iteraciones (microsegundos)
     * @param status Estado: "OK", "WARNING", "CRITICAL", "ERROR_MUTEX"
     */
    void writeLine(int iteration, double t_espera_us, double t_ejec_us,
                   double t_total_us, double periodo_us, double ts_real_us,
                   const char* status);
private:
    std::string logfile_path_;          // Ruta completa del archivo
    std::string header_;                // Header informativo
    std::vector<std::string> columns_;  // Nombres de columnas
    std::vector<int> column_widths_;    // Anchos de columnas
    std::deque<std::string> log_buffer_; // Buffer circular
    int max_lines_;                     // Máximo de líneas en buffer
    int flush_interval_;                // Intervalo de auto-flush
    int lines_since_flush_;             // Contador de líneas desde último flush
    
    /**
     * @brief Escribe el contenido completo del buffer al archivo
     */
    void writeToFile();
    
    /**
     * @brief Genera el header formateado con timestamp actualizado
     */
    std::string generateHeader() const;
};

} // namespace DiscreteSystems
