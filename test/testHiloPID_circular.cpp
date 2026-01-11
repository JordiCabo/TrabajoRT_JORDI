/**
 * @file testHiloPID_circular.cpp
 * @brief Test para validar buffer circular de 1000 iteraciones en HiloPID (v1.0.6)
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
    
    std::cout << "=== Test HiloPID v1.0.6 - Circular Buffer (>1000 iterations) ===\n\n";
    
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
    vars->e = 1.0;
    vars->u = 0.0;
    vars->yk = 0.0;
    
    // Crear PID con frecuencia alta para alcanzar >1000 iteraciones rápido
    auto pid = std::make_shared<PIDController>(
        params->kp, params->ki, params->kd, 0.001);  // Ts = 1ms
    
    std::cout << "Creating HiloPID with frequency 1000 Hz (period 1000 us)...\n";
    std::cout << "Running for 1.5 seconds to get ~1500 iterations\n";
    std::cout << "Buffer size: 1000 iterations (circular)\n\n";
    
    // Crear hilo del PID (comienza automáticamente)
    HiloPID hiloPID(pid.get(), vars.get(), params.get(), 1000, "hiloPID_circular");  // 1000 Hz para alcanzar >1000 iter
    
    std::cout << "HiloPID running...\n";
    
    // Simular por 1.5 segundos = ~1500 iteraciones
    for (int i = 0; i < 15; i++) {
        usleep(100000);  // 100ms
        
        // Actualizar error simulando una planta simple
        pthread_mutex_lock(&vars->mtx);
        double error = vars->ref - vars->yk;
        vars->e = error;
        vars->yk += vars->u * 0.001;  // Ts = 1ms
        pthread_mutex_unlock(&vars->mtx);
        
        if (i % 3 == 0) {
            int k = hiloPID.getIterations();  // Obtener iteración real del hilo
            std::cout << "t=" << (i * 0.1) << "s | ";
            std::cout << "k=" << k << " iterations\n";
        }
    }
    
    std::cout << "\nStopping HiloPID...\n";
    vars->running = false;
    
    // Esperar un poco para que el hilo termine
    usleep(100000);
    
    std::cout << "\n=== Test Complete ===\n";
    std::cout << "Expected: ~1500 iterations executed\n";
    std::cout << "Log will contain: Last 1000 iterations only (circular buffer)\n";
    std::cout << "Check logs/ directory - should show iterations 501-1500 approximately\n";
    
    return 0;
}
