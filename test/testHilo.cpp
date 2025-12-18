#include <iostream>
#include <unistd.h>
#include <mutex>
#include "TransferFunctionSystem.h"
#include "Hilo.h"
#include "Hilo2in.h"
#include "VariablesCompartidas.h"
#include "PIDController.h"
#include "DiscreteSystem.h"
#include <pthread.h>
#include "ADConverter.h"
#include "Sumador.h"
#include "DAConverter.h"
#include "HiloSignal.h"

using namespace DiscreteSystems;

 VariablesCompartidas vars;
 int N=4000;//Numero de iteraciones del bucle principal

int main() {
    // --- Variables compartidas ---
   
    vars.running = true;  // inicializamos el flag

  //-------------------------------------------------------------
  // --- Crear la referencia -----------------------------------------
  //---------------------------------------------------------------
   // Parámetros del escalón
    double Ts_step = 0.01;       // período de muestreo [s]
    double amplitude = 1.0; // amplitud del escalón
    double step_time = 0.05; // tiempo donde se produce el escalón
    double offset = 0.0;     // desplazamiento vertical
   
    //parametros para el seno
    double Ts_sin = 0.01;       // período de muestreo [s]
    double freq = 1.0; // frecuencia en Hz
    double phase = 0.0;     // fase en radianes                         

    
    SignalGenerator::StepSignal stepSignal(Ts_step, amplitude, step_time, offset);
      SignalGenerator::SineSignal sinSignal(Ts_sin,  amplitude, freq, phase, offset);

    SignalGenerator::HiloSignal hiloRef(&sinSignal,&vars.ref, &vars.running, &vars.mtx, 1/Ts_sin);
  
    //-------------------------------------------------------------
  // --- Crear la planta -----------------------------------------
  //---------------------------------------------------------------
    std::vector<double> b = {0.00995};
    std::vector<double> a = {1.0, -0.99};
    double frequency_plant = 100.0;  // Hz
    size_t bufferSize = 10;

    //quitar la frecuencia de la planta, esto lo gestiona el hilo
    TransferFunctionSystem planta(b, a, frequency_plant, bufferSize);

    //-------------------------------------------------------------
    // --- Crear hilo de la planta ---
    //-------------------------------------------------------------
    
    Hilo hiloPlanta(&planta, &vars.ua, &vars.yk,  &vars.running, &vars.mtx, frequency_plant);

    //-------------------------------------------------------------
    //Crear ADConverter-----------------------------------------------
    //-------------------------------------------------------------
    double Ts_converter = 1/frequency_plant;  // período de muestreo

    ADConverter ADconverter(Ts_converter);
    Hilo hiloAD(&ADconverter, &vars.yk, &vars.ykd, &vars.running, &vars.mtx, 1/Ts_converter);

    //-------------------------------------------------------------
// Crear PID-----------------------------------------------
//-------------------------------------------------------------

    double Kp = 5.0;
    double Ki = 3.0;
    double Kd = 0.7;
    double Ts_controller = 0.01;  // periodo de muestreo 0.1 s

    PIDController pid(Kp, Ki, Kd, Ts_controller);
    Hilo hiloPID(&pid, &vars.e, &vars.u, &vars.running, &vars.mtx, 1/Ts_controller);

    //-------------------------------------------------------------
    //Crear DAConverter-----------------------------------------------
    //-------------------------------------------------------------
    

    DAConverter DAconverter(Ts_converter);
    Hilo hiloDA(&DAconverter, &vars.u, &vars.ua, &vars.running, &vars.mtx, 1/Ts_converter); 
  
    //-------------------------------------------------------------
// Crear Sumador-----------------------------------------------
//-------------------------------------------------------------
    
    double Ts_sumador = Ts_controller;
    Sumador sumador(Ts_sumador);
    Hilo2in hiloSumador(&sumador, &vars.ref, &vars.ykd,  &vars.e, &vars.running, &vars.mtx, 1/Ts_sumador);


    // --- Bucle principal: enviar referencia y leer salida en tiempo real ---
 
    for (int k = 0; k < N; ++k) {
             // Leer salida de la planta
        double yk;
        {
            std::lock_guard<std::mutex> lock(vars.mtx);
            yk = vars.yk;
        }

        std::cout << "k=" << k
                  << " | Ref=" << vars.ref
                  << " | Planta yk=" << yk
                  << std::endl;

        usleep(5000); // 5 ms (más rápido que el hilo)
    }

    // --- Indicar al hilo que termine ---
    {
        std::lock_guard<std::mutex> lock(vars.mtx);
        vars.running = false;
    }

    // --- Recoger la terminación del hilo con pthread_join ---
   void* statusVal;
int rc = pthread_join(hiloPlanta.getThread(), &statusVal);
if (rc != 0) {
    std::cerr << "Error esperando al hilo: " << rc << std::endl;
    return 1;
}

// Recuperar el valor devuelto por el hilo
int* ret = static_cast<int*>(statusVal);
std::cout << "Hilo terminó con valor: " << *ret << std::endl;
delete ret; // liberar memoria

    return 0;
}
