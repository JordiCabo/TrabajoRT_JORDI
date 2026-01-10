/**
 * @file HiloPID.cpp
 * @brief Implementación del wrapper especializado para PID con parámetros dinámicos usando shared_ptr
 * @author Jordi + GitHub Copilot
 * @date 2026-01-10
 * @version 1.0.6 - Added timing instrumentation and trylock mechanism
 */

#include "../include/HiloPID.h"
#include "../include/PIDController.h"
#include "../include/Temporizador.h"
#include <iostream>
#include <stdexcept>
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
 * Crea un nuevo hilo pthread que ejecutará la función threadFunc,
 * iniciando la simulación del PID con actualización dinámica de parámetros.
 * 
 * @note shared_ptr incrementa referencias; el hilo mantiene co-propiedad de todos los recursos
 */
HiloPID::HiloPID(std::shared_ptr<DiscreteSystem> pid, 
                 std::shared_ptr<VariablesCompartidas> vars, 
                 std::shared_ptr<ParametrosCompartidos> params, 
                 double frequency)
    : system_(pid), vars_(vars), params_(params), frequency_(frequency), iterations_(0) {
    
    // Crear directorio logs/ si no existe
    mkdir("logs", 0755);
    
    // Inicializar buffer circular
    timing_buffer_.resize(MAX_LOG_ENTRIES);
    
    // Crear nombre de archivo con timestamp
    time_t now = time(nullptr);
    struct tm* tm_info = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", tm_info);
    
    std::ostringstream oss;
    oss << "logs/HiloPID_runtime_" << timestamp << ".txt";
    logfile_path_ = oss.str();
    
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
 * Decrementa referencias de shared_ptr automáticamente.
 */
HiloPID::~HiloPID() {
    int ret = pthread_join(thread_, nullptr);
    if (ret != 0) {
        std::cerr << "WARNING HiloPID: pthread_join failed with code " << ret << std::endl;
    }
    
    // Volcar buffer circular al archivo antes de destruir
    flushLogBuffer();
}

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
 * 1. Verifica estado de ejecución
 * 2. Intenta adquirir mutex con trylock (no bloquea)
 * 3. Lee parámetros dinámicos (kp, ki, kd) de params_
 * 4. Actualiza ganancias del PID con setGains()
 * 5. Lee error de entrada, ejecuta PID, escribe acción de control
 * 6. Registra tiempos de espera, ejecución y uso del período
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
    
    // Calcular período en microsegundos
    const double periodo_us = 1000000.0 / frequency_;
    const double threshold_80 = 0.80 * periodo_us;
    const double threshold_90 = 0.90 * periodo_us;

    // Cast a PIDController para usar setGains
    PIDController* pid = dynamic_cast<PIDController*>(system_.get());
    
    // Variables para medición de tiempos
    struct timespec t0, t1, t2;
    
    while (true) {
        iterations_++;
        
        // === INICIO MEDICIÓN CICLO ===
        clock_gettime(CLOCK_MONOTONIC, &t0);
        
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
                logTiming(iterations_, t_espera_us, 0, t_espera_us, periodo_us, "ERROR_MUTEX");
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

        // 4. Leer entrada (error), ejecutar PID, escribir salida (control)
        double input = vars_->e;              // Leer error
        double output = system_->next(input); // Ejecutar PID
        vars_->u = output;                    // Escribir acción de control
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
        logTiming(iterations_, t_espera_us, t_ejecucion_us, t_total_us, periodo_us, status);

        // 5. Dormir hasta el siguiente período absoluto (sin drift)
        timer.esperar();
    }

    pthread_exit(nullptr);
}

/**
 * @brief Registra datos de timing en el buffer circular en memoria
 * 
 * @param iteration Número de iteración
 * @param t_espera_us Tiempo de espera del mutex (microsegundos)
 * @param t_ejec_us Tiempo de ejecución de la tarea (microsegundos)
 * @param t_total_us Tiempo total del ciclo (microsegundos)
 * @param periodo_us Período de muestreo configurado (microsegundos)
 * @param status Estado: "OK", "WARNING", "CRITICAL", "ERROR_MUTEX"
 * 
 * @note Buffer circular: iteración 1001 reescribe posición 1 (índice 0)
 * @version 1.0.6
 */
void HiloPID::logTiming(int iteration, double t_espera_us, double t_ejec_us, 
                        double t_total_us, double periodo_us, const char* status) {
    // Calcular índice en buffer circular (0-999)
    size_t index = (iteration - 1) % MAX_LOG_ENTRIES;
    
    // Almacenar datos en buffer circular
    timing_buffer_[index].iteration = iteration;
    timing_buffer_[index].t_espera_us = t_espera_us;
    timing_buffer_[index].t_ejec_us = t_ejec_us;
    timing_buffer_[index].t_total_us = t_total_us;
    timing_buffer_[index].periodo_us = periodo_us;
    timing_buffer_[index].porcentaje_uso = (t_total_us / periodo_us) * 100.0;
    timing_buffer_[index].status = status;
}

/**
 * @brief Vuelca el buffer circular completo al archivo de log
 * 
 * Escribe hasta 1000 iteraciones más recientes al archivo en formato tabla.
 * Si se ejecutaron más de 1000 iteraciones, solo escribe las últimas 1000.
 * 
 * @version 1.0.6
 */
void HiloPID::flushLogBuffer() {
    std::ofstream logfile(logfile_path_);
    if (!logfile.is_open()) {
        std::cerr << "WARNING HiloPID: Could not open log file " << logfile_path_ << " for writing" << std::endl;
        return;
    }
    
    // Escribir header
    logfile << "HiloPID Runtime Performance Log (Circular Buffer - Last " << MAX_LOG_ENTRIES << " iterations)\n";
    logfile << "Frequency: " << frequency_ << " Hz\n";
    logfile << "Sample Period: " << (1000000.0 / frequency_) << " us\n";
    logfile << "Total Iterations: " << iterations_ << "\n";
    logfile << "================================================================================\n";
    logfile << std::left << std::setw(10) << "Iteration"
            << std::setw(14) << "t_espera_us"
            << std::setw(14) << "t_ejec_us"
            << std::setw(14) << "t_total_us"
            << std::setw(14) << "periodo_us"
            << std::setw(10) << "%uso"
            << std::setw(12) << "Status" << "\n";
    logfile << "--------------------------------------------------------------------------------\n";
    
    // Determinar cuántas iteraciones escribir
    size_t num_entries = (iterations_ < static_cast<int>(MAX_LOG_ENTRIES)) ? iterations_ : MAX_LOG_ENTRIES;
    
    if (iterations_ <= static_cast<int>(MAX_LOG_ENTRIES)) {
        // Menos de 1000 iteraciones: escribir en orden secuencial
        for (size_t i = 0; i < num_entries; i++) {
            const auto& data = timing_buffer_[i];
            logfile << std::left << std::setw(10) << data.iteration
                    << std::setw(14) << std::fixed << std::setprecision(2) << data.t_espera_us
                    << std::setw(14) << std::fixed << std::setprecision(2) << data.t_ejec_us
                    << std::setw(14) << std::fixed << std::setprecision(2) << data.t_total_us
                    << std::setw(14) << std::fixed << std::setprecision(2) << data.periodo_us
                    << std::setw(10) << std::fixed << std::setprecision(2) << data.porcentaje_uso
                    << std::setw(12) << data.status << "\n";
        }
    } else {
        // Más de 1000 iteraciones: escribir desde la más antigua a la más reciente
        // La iteración más antigua en el buffer está en: (iterations_ % MAX_LOG_ENTRIES)
        size_t oldest_index = iterations_ % MAX_LOG_ENTRIES;
        
        // Escribir desde oldest_index hasta el final del buffer
        for (size_t i = oldest_index; i < MAX_LOG_ENTRIES; i++) {
            const auto& data = timing_buffer_[i];
            logfile << std::left << std::setw(10) << data.iteration
                    << std::setw(14) << std::fixed << std::setprecision(2) << data.t_espera_us
                    << std::setw(14) << std::fixed << std::setprecision(2) << data.t_ejec_us
                    << std::setw(14) << std::fixed << std::setprecision(2) << data.t_total_us
                    << std::setw(14) << std::fixed << std::setprecision(2) << data.periodo_us
                    << std::setw(10) << std::fixed << std::setprecision(2) << data.porcentaje_uso
                    << std::setw(12) << data.status << "\n";
        }
        
        // Escribir desde el inicio del buffer hasta oldest_index
        for (size_t i = 0; i < oldest_index; i++) {
            const auto& data = timing_buffer_[i];
            logfile << std::left << std::setw(10) << data.iteration
                    << std::setw(14) << std::fixed << std::setprecision(2) << data.t_espera_us
                    << std::setw(14) << std::fixed << std::setprecision(2) << data.t_ejec_us
                    << std::setw(14) << std::fixed << std::setprecision(2) << data.t_total_us
                    << std::setw(14) << std::fixed << std::setprecision(2) << data.periodo_us
                    << std::setw(10) << std::fixed << std::setprecision(2) << data.porcentaje_uso
                    << std::setw(12) << data.status << "\n";
        }
    }
    
    logfile.close();
    std::cout << "HiloPID: Log flushed to " << logfile_path_ << " (" << num_entries << " iterations)" << std::endl;
}


} // namespace DiscreteSystems
