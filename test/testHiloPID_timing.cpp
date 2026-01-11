/**
 * @file testHiloPID_timing.cpp
 * @brief Test para validar instrumentación de timing en HiloPID (v1.0.6)
 * @author Jordi + GitHub Copilot
 * @date 2026-01-10
 */

#include <iostream>
#include <memory>
#include <unistd.h>
#include "../include/PIDController.h"
#include "../include/HiloPID.h"
#include "../include/VariablesCompartidas.h"
#include "../include/ParametrosCompartidos.h"

int main() {
    using namespace DiscreteSystems;
    
    std::cout << "=== Test HiloPID v1.0.6 - Timing Instrumentation ===\n\n";
    
    // Crear variables compartidas
    auto vars = std::make_shared<VariablesCompartidas>();
    auto params = std::make_shared<ParametrosCompartidos>();
    
    // Configurar parámetros del PID
    params->kp = 5.0;
    params->ki = 3.0;
    params->kd = 0.7;
    params->setpoint = 1.0;
    
    // Configurar variables iniciales
    vars->running = true;
    vars->ref = 1.0;
    vars->e = 1.0;   // Error inicial
    vars->u = 0.0;
    vars->yk = 0.0;
    
    // Crear PID con frecuencia de 100 Hz (período 10ms)
    auto pid = std::make_shared<PIDController>(
        params->kp, params->ki, params->kd, 0.01);  // Ts = 10ms
    
    std::cout << "Creating HiloPID with frequency 100 Hz (period 10000 us)...\n";
    std::cout << "Logging to logs/HiloPID_runtime_YYYYMMDD_HHMMSS.txt\n\n";
    
    // Crear hilo del PID (comienza automáticamente)
    HiloPID hiloPID(pid.get(), vars.get(), params.get(), 100, "hiloPID_timing");  // 100 Hz
    
    std::cout << "HiloPID running. Simulating for 2 seconds...\n";
    std::cout << "Expected iterations: ~200\n\n";
    
    // Simular por 2 segundos
    for (int i = 0; i < 20; i++) {
        usleep(100000);  // 100ms
        
        // Actualizar error simulando una planta simple
        pthread_mutex_lock(&vars->mtx);
        double error = vars->ref - vars->yk;
        vars->e = error;
        
        // Simular respuesta de la planta: yk[k+1] = yk[k] + u[k]*Ts
        vars->yk += vars->u * 0.01;
        pthread_mutex_unlock(&vars->mtx);
        
        if (i % 5 == 0) {
            int k = hiloPID.getIterations();  // Obtener iteración real del hilo
            std::cout << "k=" << k << " | ";
            std::cout << "t=" << (i * 0.1) << "s | ";
            std::cout << "ref=" << vars->ref << " | ";
            std::cout << "yk=" << vars->yk << " | ";
            std::cout << "e=" << vars->e << " | ";
            std::cout << "u=" << vars->u << "\n";
        }
    }
    
    std::cout << "\nStopping HiloPID...\n";
    vars->running = false;
    
    // Esperar un poco para que el hilo termine
    usleep(100000);
    
    std::cout << "\n=== Test Complete ===\n";
    std::cout << "Check logs/ directory for timing data\n";
    std::cout << "Format: Iteration | t_espera_us | t_ejec_us | t_total_us | periodo_us | %uso | Status\n";
    
    return 0;
}
