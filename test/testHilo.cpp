#include <iostream>
#include <unistd.h>
#include <mutex>
#include "TransferFunctionSystem.h"
#include "Hilo.h"
#include "Hilo2in.h"
#include "HiloPID.h"
#include "VariablesCompartidas.h"
#include "ParametrosCompartidos.h"
#include "PIDController.h"
#include "DiscreteSystem.h"
#include <pthread.h>
#include "ADConverter.h"
#include "Sumador.h"
#include "DAConverter.h"
#include "HiloSignal.h"
#include "SignalSwitch.h"
#include "HiloSwitch.h"
#include "Transmisor.h"
#include "HiloTransmisor.h"
#include "Receptor.h"
#include "HiloReceptor.h"
#include "InterruptorArranque.h"
#include "HiloIntArranque.h"

using namespace DiscreteSystems;

 VariablesCompartidas vars;
 ParametrosCompartidos params;
 

int main() {
    // --- Variablefalse;  // inicialmente en false
    
    // --- Crear InterruptorArranque y HiloIntArranque ---
    InterruptorArranque interruptor;

    // Activar running a través del interruptor
    interruptor.setRun(1);

    // Crear el hilo del interruptor de arranque/paro
    HiloIntArranque hiloInterruptor(&interruptor, &vars.running, &vars.mtx);
    
   
    

  //-------------------------------------------------------------
  // --- Crear la referencia (SignalSwitch) -----------------------------------------
  //---------------------------------------------------------------
   // Parámetros del escalón
    double Ts_signal = 0.01;     // período de muestreo común [s]
    double amplitude = 1.0;     // amplitud
    double step_time = 0.05;     // tiempo del escalón
    double offset = 0.0;         // desplazamiento vertical
   
    // Parámetros para el seno
    double freq = 1.0;           // frecuencia en Hz
    double phase = 0.0;          // fase en radianes
    double sinAmp=10.0;
    
    // Parámetros para PWM
    double duty = 0.5;           // ciclo de trabajo
    double period_pwm = 1.0;     // período de PWM [s]

    // Crear las 3 señales como shared_ptr
    auto stepSignal = std::make_shared<SignalGenerator::StepSignal>(Ts_signal, amplitude, step_time, offset);
    auto sinSignal = std::make_shared<SignalGenerator::SineSignal>(Ts_signal, sinAmp, freq, phase, offset);
    auto pwmSignal = std::make_shared<SignalGenerator::PwmSignal>(Ts_signal, amplitude, duty, period_pwm, offset);

    // Crear SignalSwitch: 0=step, 1=pwm, 2=sine (selector inicial = 0: escalón)
    SignalGenerator::SignalSwitch signalSwitch(stepSignal, pwmSignal, sinSignal, 0);
    
   
    
    // Crear HiloSwitch para ejecutar el switch periódicamente
    HiloSwitch hiloRef(&signalSwitch, &vars.ref, &vars.running, &vars.mtx, &params, 1/Ts_signal);
  
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

    // Inicializar parámetros compartidos con valores por defecto
    pthread_mutex_lock(&params.mtx);
    params.kp = 5.0;
    params.ki = 3.0;
    params.kd = 0.7;
    params.setpoint = 1.0;  // igual a la amplitud del escalón
    pthread_mutex_unlock(&params.mtx);

    double Ts_controller = 0.01;  // periodo de muestreo 0.01 s

    PIDController pid(params.kp, params.ki, params.kd, Ts_controller);
    
    // Usar HiloPID que lee parámetros dinámicamente
    HiloPID hiloPID(&pid, &vars, &params, 1/Ts_controller);

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

    // --- Crear transmisor para enviar datos via IPC ---
    Transmisor transmisor(&vars);
    if (!transmisor.inicializar()) {
        std::cerr << "Error: No se pudo inicializar el Transmisor" << std::endl;
        vars.running = false;
        return 1;
    }
    std::cout << "Transmisor inicializado correctamente" << std::endl;

    // --- Crear hilo de transmisión a 50 Hz ---
    HiloTransmisor hiloTransmisor(&transmisor, &vars.running, &vars.mtx, 50.0);
    std::cout << "Hilo de transmisión iniciado a 50 Hz" << std::endl;

    // --- Crear receptor para recibir parámetros via IPC ---
    Receptor receptor(&params);
    if (!receptor.inicializar()) {
        std::cerr << "Error: No se pudo inicializar el Receptor" << std::endl;
        vars.running = false;
        return 1;
    }
    std::cout << "Receptor inicializado correctamente" << std::endl;

    // --- Crear hilo de recepción a 50 Hz ---
    HiloReceptor hiloReceptor(&receptor, &vars.running, &vars.mtx, 50.0);
    std::cout << "Hilo de recepción iniciado a 50 Hz" << std::endl;

    // --- Bucle principal: monitorizar salida en tiempo real ---
    int k=0;
    while(true) {
        bool running_now;
        pthread_mutex_lock(&vars.mtx);
        running_now = vars.running;
        pthread_mutex_unlock(&vars.mtx);
        if (!running_now) break;
        // Leer salida de la planta
        double yk;
        pthread_mutex_lock(&vars.mtx);
        yk = vars.yk;
        pthread_mutex_unlock(&vars.mtx);

        // Leer parámetros actuales del PID y tipo de señal
        double kp_actual, ki_actual, kd_actual, setpoint_actual;
        int signal_type_actual;
        pthread_mutex_lock(&params.mtx);
        kp_actual = params.kp;
        ki_actual = params.ki;
        kd_actual = params.kd;
        setpoint_actual = params.setpoint;
        signal_type_actual = params.signal_type;
        pthread_mutex_unlock(&params.mtx);

        std::cout << "k=" << k
                  << " | Ref=" << vars.ref
                  << " | u=" << vars.u
                  << " | yk=" << yk
                  << " | Kp=" << kp_actual
                  << " | Ki=" << ki_actual
                  << " | Kd=" << kd_actual
                  << " | Setpoint=" << setpoint_actual
                  << " | Signal=" << signal_type_actual << " (0=step,1=sine,2=pwm)"
                  << " | t=" << transmisor.getTiempoTranscurrido() << "s"
                  << std::endl;
        k++;
        usleep(50000); // 50 ms entre impresiones
    }

   

    // --- Esperar terminación de todos los hilos ---
    pthread_join(hiloRef.getThread(), nullptr);
    pthread_join(hiloPlanta.getThread(), nullptr);
    pthread_join(hiloAD.getThread(), nullptr);
    pthread_join(hiloPID.getThread(), nullptr);
    pthread_join(hiloDA.getThread(), nullptr);
    pthread_join(hiloSumador.getThread(), nullptr);
    pthread_join(hiloTransmisor.getThread(), nullptr);
    pthread_join(hiloReceptor.getThread(), nullptr);

    // Cerrar transmisor y receptor
    transmisor.cerrar();
    receptor.cerrar();

    return 0;
}
