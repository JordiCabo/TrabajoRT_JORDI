/**
 * @file RuntimeLogger.cpp
 * @brief Implementación del sistema de logging genérico con buffer circular
 * @author Jordi + GitHub Copilot
 * @date 2026-01-11
 * @version 1.0.7
 */

#include "../include/RuntimeLogger.h"
#include <iostream>

namespace DiscreteSystems {

RuntimeLogger::RuntimeLogger(const std::string& prefix, int max_lines, 
                             const std::string& log_dir)
    : max_lines_(max_lines), flush_interval_(100), lines_since_flush_(0)
{
    // Crear directorio de logs si no existe
    mkdir(log_dir.c_str(), 0755);
    
    // Crear nombre de archivo con timestamp
    time_t now = time(nullptr);
    struct tm* tm_info = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", tm_info);
    
    std::ostringstream oss;
    oss << log_dir << "/" << prefix << "_runtime_" << timestamp << ".txt";
    logfile_path_ = oss.str();
}

RuntimeLogger::~RuntimeLogger() {
    flush();  // Escribir buffer final antes de destruir
}

void RuntimeLogger::setHeader(const std::string& header) {
    header_ = header;
}

void RuntimeLogger::setColumns(const std::vector<std::string>& columns, 
                               const std::vector<int>& widths) {
    columns_ = columns;
    column_widths_ = widths;
    
    // Si no se especifican anchos, usar 14 por defecto
    if (column_widths_.empty()) {
        column_widths_.assign(columns_.size(), 14);
    }
}

void RuntimeLogger::writeLine(const std::string& line, bool force_flush) {
    // Añadir al buffer circular
    if (log_buffer_.size() >= static_cast<size_t>(max_lines_)) {
        log_buffer_.pop_front();  // Eliminar la más antigua
    }
    log_buffer_.push_back(line);
    
    // Auto-flush si se alcanza el intervalo
    lines_since_flush_++;
    if (force_flush || (flush_interval_ > 0 && lines_since_flush_ >= flush_interval_)) {
        flush();
    }
}

void RuntimeLogger::flush() {
    writeToFile();
    lines_since_flush_ = 0;
}

void RuntimeLogger::setFlushInterval(int interval) {
    flush_interval_ = interval;
}

void RuntimeLogger::writeToFile() {
    std::ofstream logfile(logfile_path_, std::ios::trunc);  // Sobrescribir
    if (!logfile.is_open()) {
        std::cerr << "WARNING RuntimeLogger: Could not open " << logfile_path_ << std::endl;
        return;
    }
    
    // Escribir header actualizado
    logfile << generateHeader();
    
    // Escribir todas las líneas del buffer
    for (const auto& line : log_buffer_) {
        logfile << line;
    }
    
    logfile.close();
}

std::string RuntimeLogger::generateHeader() const {
    std::ostringstream header_stream;
    
    // Timestamp actualizado
    time_t now = time(nullptr);
    struct tm* tm_info = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    // Header personalizado
    if (!header_.empty()) {
        header_stream << header_ << "\n";
    }
    
    header_stream << "Last Updated: " << timestamp << "\n";
    header_stream << "Buffer Size: " << log_buffer_.size() << "/" << max_lines_ << " lines\n";
    header_stream << std::string(80, '=') << "\n";
    
    // Columnas
    if (!columns_.empty()) {
        for (size_t i = 0; i < columns_.size(); i++) {
            int width = (i < column_widths_.size()) ? column_widths_[i] : 14;
            header_stream << std::left << std::setw(width) << columns_[i];
        }
        header_stream << "\n";
        header_stream << std::string(80, '-') << "\n";
    }
    
    return header_stream.str();
}

/**
 * @brief Inicializa el logger para HiloPID con columnas y header específico
 */
void RuntimeLogger::initializeHiloPID(double frequency) {
    // Configurar header
    std::ostringstream header;
    header << "HiloPID Runtime Performance Log\n";
    header << "Frequency: " << frequency << " Hz\n";
    header << "Sample Period: " << (1000000.0 / frequency) << " us";
    setHeader(header.str());
    
    // Configurar columnas con anchos específicos
    std::vector<std::string> cols = {"Iteration", "t_espera_us", "t_ejec_us", "t_total_us", "periodo_us", 
                                     "Ts_Real_us", "drift_us", "%error_Ts", "%uso", "Status"};
    std::vector<int> widths = {10, 14, 14, 14, 14, 14, 14, 12, 10, 12};
    setColumns(cols, widths);
}

/**
 * @brief Inicializa el logger para Hilo con columnas y header específico
 */
void RuntimeLogger::initializeHilo(double frequency) {
    // Configurar header
    std::ostringstream header;
    header << "Hilo Runtime Performance Log\n";
    header << "Frequency: " << frequency << " Hz\n";
    header << "Sample Period: " << (1000000.0 / frequency) << " us";
    setHeader(header.str());
    
    // Configurar columnas con anchos específicos
    std::vector<std::string> cols = {"Iteration", "t_espera_us", "t_ejec_us", "t_total_us", "periodo_us", 
                                     "Ts_Real_us", "drift_us", "%error_Ts", "%uso", "Status"};
    std::vector<int> widths = {10, 14, 14, 14, 14, 14, 14, 12, 10, 12};
    setColumns(cols, widths);
}

/**
 * @brief Escribe una línea de timing con formateo automático (versión sobrecargada)
 */
void RuntimeLogger::writeLine(int iteration, double t_espera_us, double t_ejec_us,
                              double t_total_us, double periodo_us, double ts_real_us,
                              const char* status) {
    double porcentaje_uso = (t_total_us / periodo_us) * 100.0;
    double drift_us = ts_real_us - periodo_us;
    double error_ts = (drift_us / periodo_us) * 100.0;
    
    // Construir línea formateada con los anchos específicos de HiloPID
    std::ostringstream line;
    line << std::left << std::setw(10) << iteration
         << std::setw(14) << std::fixed << std::setprecision(2) << t_espera_us
         << std::setw(14) << std::fixed << std::setprecision(2) << t_ejec_us
         << std::setw(14) << std::fixed << std::setprecision(2) << t_total_us
         << std::setw(14) << std::fixed << std::setprecision(2) << periodo_us
         << std::setw(14) << std::fixed << std::setprecision(2) << ts_real_us
         << std::setw(14) << std::fixed << std::setprecision(2) << drift_us
         << std::setw(12) << std::fixed << std::setprecision(2) << error_ts
         << std::setw(10) << std::fixed << std::setprecision(2) << porcentaje_uso
         << std::setw(12) << status << "\n";
    
    writeLine(line.str());
}

} // namespace DiscreteSystems
