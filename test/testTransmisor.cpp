/*
 * @file testTransmisor.cpp
 * 
 * @author Jordi
 * @author GitHub Copilot (asistencia)
 * 
 * @brief Test de la clase Transmisor
 * 
 * Prueba unitaria para verificar:
 *  - Inicialización del Transmisor
 *  - Envío de ref, u, yk desde VariablesCompartidas
 *  - Gestión automática de timestamps
 *  - Sincronización con mutex
 * 
 * @usage ./bin/testTransmisor
 *        (En otro terminal: ./bin/test_receive)
 * 
 * Compilar: cd build && cmake .. && make testTransmisor
 */

#include "Transmisor.h"
#include "VariablesCompartidas.h"
#include <iostream>
#include <unistd.h>
#include <cmath>

int main() {
    std::cout << "=== Test de Clase Transmisor ===" << std::endl;
    
    // Crear variables compartidas
    VariablesCompartidas vars;
    
    // Crear transmisor
    Transmisor tx(&vars);
    
    // Inicializar
    if (!tx.inicializar()) {
        std::cerr << "Error: No se pudo inicializar el Transmisor" << std::endl;
        return 1;
    }
    
    std::cout << "\nSimulando lazo de control..." << std::endl;
    std::cout << "Ejecutar test_receive en otro terminal para ver los datos\n" << std::endl;
    
    // Simular 100 ciclos de control
    vars.running = true;
    
    // Condición inicial
    pthread_mutex_lock(&vars.mtx);
    vars.ref = 1.0;
    vars.yk = 0.0;
    vars.u = 0.0;
    pthread_mutex_unlock(&vars.mtx);
    
    for (int k = 0; k < 100; k++) {
        double error, u_local, yk_local, ref_local;
        
        // Leer variables compartidas
        pthread_mutex_lock(&vars.mtx);
        ref_local = vars.ref;
        yk_local = vars.yk;
        pthread_mutex_unlock(&vars.mtx);
        
        // Simular error
        error = ref_local - yk_local;
        
        // Simular acción de control (P simple)
        double Kp = 1.0;
        u_local = Kp * error;
        
        // Simular planta de primer orden: y[k+1] = 0.9*y[k] + 0.1*u[k]
        yk_local = 0.9 * yk_local + 0.1 * u_local;
        
        // Actualizar variables compartidas
        pthread_mutex_lock(&vars.mtx);
        vars.u = u_local;
        vars.yk = yk_local;
        pthread_mutex_unlock(&vars.mtx);
        
        // Enviar datos (lee vars->ref, vars->u, vars->yk bajo mutex)
        if (tx.enviar()) {
            std::cout << "Ciclo " << k << ": ";
            std::cout << "ref=" << ref_local << ", ";
            std::cout << "u=" << u_local << ", ";
            std::cout << "yk=" << yk_local << ", ";
            std::cout << "t=" << tx.getTiempoTranscurrido() << "s" << std::endl;
        } else {
            std::cerr << "Error enviando en ciclo " << k << std::endl;
        }
        
        usleep(50000);  // 50ms (20 Hz)
    }
    
    std::cout << "\nSimulación completada" << std::endl;
    
    vars.running = false;
    
    // Cerrar (también se cierra automáticamente en destructor)
    tx.cerrar();
    
    return 0;
}
