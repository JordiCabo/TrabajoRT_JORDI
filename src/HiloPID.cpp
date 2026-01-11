/**
 * @file HiloPID.cpp
 * @brief Implementación del wrapper especializado para PID con parámetros dinámicos
 * @author Jordi + GitHub Copilot
 * @date 2026-01-11
 * @version 1.0.7 - Refactored to use RuntimeLogger
 */

#include "../include/HiloPID.h"
#include "../include/PIDController.h"
#include "../include/Temporizador.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <csignal>
#include <errno.h>

namespace DiscreteSystems {

/**
 * @brief Constructor que crea e inicia el hilo pthread
 * 
 * Inicializa el RuntimeLogger con configuración de HiloPID y crea
 * el hilo pthread que ejecutará el PID con instrumentación de timing.
 */
HiloPID::HiloPID(DiscreteSystem* pid, VariablesCompartidas* vars, 
                 ParametrosCompartidos* params, double frequency,
                 const std::string& log_prefix)
    : system_(pid), vars_(vars), params_(params), frequency_(frequency), 
    iterations_(0), logger_(log_prefix, 1000)
{
    // Inicializar logger con configuración específica de HiloPID
    logger_.initializeHiloPID(frequency);
    
    std::cout << "HiloPID log: " << logger_.getLogPath() << std::endl;
    
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
 * RuntimeLogger escribe el buffer final automáticamente en su destructor.
 */
HiloPID::~HiloPID() {
    int ret = pthread_join(thread_, nullptr);
    if (ret != 0) {
        std::cerr << "[HiloPID] Error: pthread_join falló con código " << ret << std::endl;
    }
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
                logger_.writeLine(iterations_, t_espera_us, 0, t_espera_us, periodo_us, ts_real_us, "ERROR_MUTEX");
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
        
        // 2. Leer parámetros dinámicos (kp, ki, kd) con timeout de 20% período
        double timeout_20pct = 0.2 * periodo_us;
        struct timespec timeout_params;
        clock_gettime(CLOCK_MONOTONIC, &timeout_params);
        long timeout_ns = (long)(timeout_20pct * 1000);
        timeout_params.tv_nsec += timeout_ns;
        if (timeout_params.tv_nsec >= 1000000000L) {
            timeout_params.tv_sec += timeout_params.tv_nsec / 1000000000L;
            timeout_params.tv_nsec %= 1000000000L;
        }
        
        double kp = params_->kp;  // cache por defecto
        double ki = params_->ki;
        double kd = params_->kd;
        
        int ret_params = pthread_mutex_timedlock(&params_->mtx, &timeout_params);
        if (ret_params == 0) {
            // Lock adquirido, leer parámetros frescos
            kp = params_->kp;
            ki = params_->ki;
            kd = params_->kd;
            pthread_mutex_unlock(&params_->mtx);
        } else if (ret_params == ETIMEDOUT) {
            logger_.writeLine(iterations_, t_espera_us, 0, t_espera_us, periodo_us, ts_real_us, "ERROR_TIMEDLOCK_PARAMS");
            // Continuar con parámetros anteriores (cache)
        } else {
            std::cerr << "ERROR HiloPID: pthread_mutex_timedlock(params) failed with code " << ret_params << std::endl;
        }

        // 3. Actualizar ganancias del PID (fuera de sección crítica)
        if (pid != nullptr) {
            pid->setGains(kp, ki, kd);
        }

        // 4. Ejecutar PID (no necesita mutex)
        double output = system_->next(input);
        
        // 5. Escribir acción de control (requiere mutex con timeout de 20% período)
        struct timespec timeout_output;
        clock_gettime(CLOCK_MONOTONIC, &timeout_output);
        timeout_output.tv_nsec += timeout_ns;
        if (timeout_output.tv_nsec >= 1000000000L) {
            timeout_output.tv_sec += timeout_output.tv_nsec / 1000000000L;
            timeout_output.tv_nsec %= 1000000000L;
        }
        
        int ret_output = pthread_mutex_timedlock(&vars_->mtx, &timeout_output);
        if (ret_output == 0) {
            vars_->u = output;
            pthread_mutex_unlock(&vars_->mtx);
        } else if (ret_output == ETIMEDOUT) {
            logger_.writeLine(iterations_, t_espera_us, 0, t_espera_us, periodo_us, ts_real_us, "ERROR_TIMEDLOCK_OUTPUT");
            // No escribir si timeout: control anterior se mantiene
        } else {
            std::cerr << "ERROR HiloPID: pthread_mutex_timedlock(output) failed with code " << ret_output << std::endl;
        }
        
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
        logger_.writeLine(iterations_, t_espera_us, t_ejecucion_us, t_total_us, 
                          periodo_us, ts_real_us, status);

        // 5. Dormir hasta el siguiente período absoluto (sin drift)
        timer.esperar();
    }

    pthread_exit(nullptr);
}

} // namespace DiscreteSystems
