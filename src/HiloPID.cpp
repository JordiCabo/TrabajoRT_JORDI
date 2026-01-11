/**
 * @file HiloPID.cpp
 * @brief Implementación del wrapper especializado para PID con parámetros dinámicos
 * @author Jordi + GitHub Copilot
 * @date 2026-01-11
 * @version 1.0.6 - Added timing instrumentation and logging
 */

#include "../include/HiloPID.h"
#include "../include/PIDController.h"
#include "../include/Temporizador.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <csignal>
#include <sys/stat.h>
#include <errno.h>

namespace DiscreteSystems {

/**
 * @brief Constructor que crea e inicia el hilo pthread
 * 
 * Crea el directorio logs/ si no existe, genera un archivo de log con timestamp
 * y registra información de configuración (frecuencia, período de muestreo).
 * Posteriormente crea el hilo pthread que ejecutará el PID.
 */
HiloPID::HiloPID(DiscreteSystem* pid, VariablesCompartidas* vars, 
                 ParametrosCompartidos* params, double frequency)
    : system_(pid), vars_(vars), params_(params), frequency_(frequency), iterations_(0)
{
    // Crear directorio logs/ en el directorio raíz si no existe
    mkdir("../logs", 0755);
    
    // Crear nombre de archivo con timestamp
    time_t now = time(nullptr);
    struct tm* tm_info = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", tm_info);
    
    std::ostringstream oss;
    oss << "../logs/HiloPID_runtime_" << timestamp << ".txt";
    logfile_path_ = oss.str();
    
    // Crear archivo y escribir header
    std::ofstream logfile(logfile_path_);
    if (logfile.is_open()) {
        logfile << "HiloPID Runtime Performance Log\n";
        logfile << "Frequency: " << frequency << " Hz\n";
        logfile << "Sample Period: " << (1000000.0 / frequency) << " us\n";
        logfile << "Timestamp: " << timestamp << "\n";
        logfile << "================================================================================\n";
        logfile << std::left << std::setw(10) << "Iteration"
                << std::setw(14) << "t_espera_us"
                << std::setw(14) << "t_ejec_us"
                << std::setw(14) << "t_total_us"
                << std::setw(14) << "periodo_us"
                << std::setw(14) << "Ts_Real_us"
                << std::setw(14) << "drift_us"
                << std::setw(12) << "%error_Ts"
                << std::setw(10) << "%uso"
                << std::setw(12) << "Status" << "\n";
        logfile << "--------------------------------------------------------------------------------\n";
        logfile.close();
    } else {
        std::cerr << "WARNING HiloPID: Could not create log file " << logfile_path_ << std::endl;
    }
    
    int ret = pthread_create(&thread_, nullptr, &HiloPID::threadFunc, this);
    if (ret != 0) {
        std::cerr << "ERROR HiloPID: pthread_create failed with code " << ret << std::endl;
        throw std::runtime_error("HiloPID: Failed to create thread");
    }
}

/**
 * @brief Destructor que espera a que termine el hilo
 * 
 * Ejecuta pthread_join() para asegurar que el hilo finaliza
 * correctamente antes de destruir el objeto HiloPID.
 */
HiloPID::~HiloPID() {
    int ret = pthread_join(thread_, nullptr);
    if (ret != 0) {
        std::cerr << "[HiloPID] Error: pthread_join falló con código " << ret << std::endl;
    }    // Escribir el buffer final al archivo antes de destruir
    writeLogFile();}

/**
 * @brief Función estática de punto de entrada del hilo pthread
 * 
 * @param arg Puntero a this (el objeto HiloPID)
 * @return nullptr
 * 
 * Esta función es llamada por pthread; castea el argumento a HiloPID*
 * y invoca el método privado run().
 */
void* HiloPID::threadFunc(void* arg) {
    HiloPID* self = static_cast<HiloPID*>(arg);
    self->run();
    return nullptr;
}

/**
 * @brief Loop principal de ejecución del hilo a frecuencia fija con instrumentación de timing
 * 
 * @version 1.0.6 - Added timing instrumentation and trylock mechanism
 * 
 * Ejecuta el PID en bucle a la frecuencia especificada mientras
 * vars_->running sea true. En cada ciclo:
 * 1. Verifica estado de ejecución con trylock (no bloqueante)
 * 2. Lee parámetros dinámicos (kp, ki, kd) de params_
 * 3. Actualiza ganancias del PID con setGains()
 * 4. Lee error de entrada, ejecuta PID, escribe acción de control
 * 5. Registra tiempos de espera, ejecución y uso del período
 * 
 * Timing crítico:
 * - t_espera: tiempo esperando el mutex (debe ser < 80% período)
 * - t_ejecucion: tiempo de cómputo del PID
 * - t_total: tiempo ciclo completo (debe ser < 100% período)
 * 
 * Si t_espera > 80% período → ERROR, salta iteración (mutex contention)
 * Si t_total > 100% período → CRITICAL, deadline missed
 * Si t_total > 90% período → WARNING, near deadline
 * 
 * @invariant Período de ejecución = 1/frequency_ segundos
 * @invariant Acceso a vars_ y params_ protegido por sus respectivos mutex
 */
void HiloPID::run() {
    // Crear temporizador con retardo absoluto (evita drift acumulativo)
    Temporizador timer(frequency_);
    
    if (!system_ || !vars_ || !params_) {
        return;
    }
    
    // Calcular período en microsegundos
    const double periodo_us = 1000000.0 / frequency_;
    const double threshold_80 = 0.80 * periodo_us;
    const double threshold_90 = 0.90 * periodo_us;

    // Cast a PIDController para usar setGains
    PIDController* pid = dynamic_cast<PIDController*>(system_);
    
    // Variables para medición de tiempos
    struct timespec t0, t1, t2;
    
    // Inicializar timestamp anterior
    clock_gettime(CLOCK_MONOTONIC, &t_prev_iteration_);
    
    while (true) {
        iterations_++;
        
        // === INICIO MEDICIÓN CICLO ===
        clock_gettime(CLOCK_MONOTONIC, &t0);
        
        // Calcular Ts real (tiempo desde iteración anterior)
        double ts_real_us = (t0.tv_sec - t_prev_iteration_.tv_sec) * 1000000.0 +
                            (t0.tv_nsec - t_prev_iteration_.tv_nsec) / 1000.0;
        t_prev_iteration_ = t0;  // Actualizar timestamp anterior
        
        // 1. Verificar si debe seguir ejecutando (con trylock)
        int ret_trylock = pthread_mutex_trylock(&vars_->mtx);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        
        // Calcular tiempo de espera del mutex
        double t_espera_us = (t1.tv_sec - t0.tv_sec) * 1000000.0 + 
                             (t1.tv_nsec - t0.tv_nsec) / 1000.0;
        
        if (ret_trylock == EBUSY) {
            // Mutex bloqueado, verificar si supera 80% del período
            if (t_espera_us > threshold_80) {
                std::cerr << "ERROR HiloPID [iter " << iterations_ 
                          << "]: Mutex locked for " << t_espera_us 
                          << " us (>" << threshold_80 << " us, 80% period). Skipping iteration.\n";
                
                // Log del error
                logTiming(iterations_, t_espera_us, 0, t_espera_us, periodo_us, ts_real_us, "ERROR_MUTEX");
            }
            // Saltar iteración y esperar al siguiente período
            timer.esperar();
            continue;
        } else if (ret_trylock != 0) {
            std::cerr << "ERROR HiloPID: pthread_mutex_trylock failed with code " << ret_trylock << std::endl;
            timer.esperar();
            continue;
        }
        
        // Mutex adquirido correctamente
        bool running = vars_->running;
        
        if (!running) {
            pthread_mutex_unlock(&vars_->mtx);
            break; // salir si se recibió SIGINT/SIGTERM o running es false
        }
        
        // Leer error de entrada (mutex ya bloqueado)
        double input = vars_->e;
        pthread_mutex_unlock(&vars_->mtx);
        
        // === EJECUCIÓN DE TAREA ===
        
        // 2. Leer parámetros dinámicos (kp, ki, kd) con su propio mutex
        pthread_mutex_lock(&params_->mtx);
        double kp = params_->kp;
        double ki = params_->ki;
        double kd = params_->kd;
        pthread_mutex_unlock(&params_->mtx);

        // 3. Actualizar ganancias del PID (fuera de sección crítica)
        if (pid != nullptr) {
            pid->setGains(kp, ki, kd);
        }

        // 4. Ejecutar PID (no necesita mutex)
        double output = system_->next(input);
        
        // 5. Escribir acción de control (requiere mutex)
        pthread_mutex_lock(&vars_->mtx);
        vars_->u = output;
        pthread_mutex_unlock(&vars_->mtx);
        
        // === FIN MEDICIÓN CICLO ===
        clock_gettime(CLOCK_MONOTONIC, &t2);
        
        // Calcular tiempos
        double t_ejecucion_us = (t2.tv_sec - t1.tv_sec) * 1000000.0 + 
                                (t2.tv_nsec - t1.tv_nsec) / 1000.0;
        double t_total_us = (t2.tv_sec - t0.tv_sec) * 1000000.0 + 
                            (t2.tv_nsec - t0.tv_nsec) / 1000.0;
        
        // Determinar estado basado en umbrales
        const char* status;
        if (t_total_us > periodo_us) {
            status = "CRITICAL";
            std::cerr << "CRITICAL HiloPID [iter " << iterations_ 
                      << "]: Deadline missed! t_total=" << t_total_us 
                      << " us > period=" << periodo_us << " us\n";
        } else if (t_total_us > threshold_90) {
            status = "WARNING";
            std::cerr << "WARNING HiloPID [iter " << iterations_ 
                      << "]: Near deadline (>90%). t_total=" << t_total_us << " us\n";
        } else {
            status = "OK";
        }
        
        // Log de timing
        logTiming(iterations_, t_espera_us, t_ejecucion_us, t_total_us, periodo_us, ts_real_us, status);

        // 5. Dormir hasta el siguiente período absoluto (sin drift)
        timer.esperar();
    }

    pthread_exit(nullptr);
}

/**
 * @brief Registra datos de timing en buffer circular de 1000 líneas
 * 
 * Mantiene solo las últimas 1000 mediciones en memoria. Cuando se alcanza el límite,
 * sobrescribe la línea más antigua (comportamiento de buffer circular).
 * 
 * @param iteration Número de iteración
 * @param t_espera_us Tiempo de espera del mutex (microsegundos)
 * @param t_ejec_us Tiempo de ejecución de la tarea (microsegundos)
 * @param t_total_us Tiempo total del ciclo (microsegundos)
 * @param periodo_us Período de muestreo configurado (microsegundos)
 * @param ts_real_us Período real medido entre iteraciones (microsegundos)
 * @param status Estado: "OK", "WARNING", "CRITICAL", "ERROR_MUTEX"
 * 
 * @version 1.0.6
 */
void HiloPID::logTiming(int iteration, double t_espera_us, double t_ejec_us, 
                        double t_total_us, double periodo_us, double ts_real_us, const char* status) {
    double porcentaje_uso = (t_total_us / periodo_us) * 100.0;
    double drift_us = ts_real_us - periodo_us;
    double error_ts = (drift_us / periodo_us) * 100.0;
    
    // Construir línea en memoria
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
    
    // Añadir al buffer circular (mantener solo últimas 1000 líneas)
    if (log_buffer_.size() >= MAX_LOG_LINES) {
        log_buffer_.pop_front();  // Eliminar la más antigua
    }
    log_buffer_.push_back(line.str());
    
    // Escribir al archivo cada 100 iteraciones para evitar I/O excesivo
    if (iteration % 100 == 0) {
        writeLogFile();
    }
}

/**
 * @brief Escribe el contenido completo del buffer al archivo de log
 * 
 * Reescribe todo el archivo con el header y las líneas del buffer.
 * Llamado periódicamente (cada 100 iteraciones) y en el destructor.
 * 
 * @version 1.0.6
 */
void HiloPID::writeLogFile() {
    std::ofstream logfile(logfile_path_, std::ios::trunc);  // Sobrescribir archivo
    if (logfile.is_open()) {
        // Reescribir header
        time_t now = time(nullptr);
        struct tm* tm_info = localtime(&now);
        char timestamp[32];
        strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", tm_info);
        
        logfile << "HiloPID Runtime Performance Log\n";
        logfile << "Frequency: " << frequency_ << " Hz\n";
        logfile << "Sample Period: " << (1000000.0 / frequency_) << " us\n";
        logfile << "Last Updated: " << timestamp << "\n";
        logfile << "================================================================================\n";
        logfile << std::left << std::setw(10) << "Iteration"
                << std::setw(14) << "t_espera_us"
                << std::setw(14) << "t_ejec_us"
                << std::setw(14) << "t_total_us"
                << std::setw(14) << "periodo_us"
                << std::setw(14) << "Ts_Real_us"
                << std::setw(14) << "drift_us"
                << std::setw(12) << "%error_Ts"
                << std::setw(10) << "%uso"
                << std::setw(12) << "Status" << "\n";
        logfile << "--------------------------------------------------------------------------------\n";
        
        // Escribir todas las líneas del buffer
        for (const auto& line : log_buffer_) {
            logfile << line;
        }
        
        logfile.close();
    }
}

} // namespace DiscreteSystems
