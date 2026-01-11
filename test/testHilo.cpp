#include <iostream>
#include <unistd.h>
#include <mutex>
#include <memory>
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
#include "Discretizer.h"

using namespace DiscreteSystems;

int main() {
    // --- Crear variables compartidas como shared_ptr ---
    auto vars = std::make_shared<VariablesCompartidas>();
    auto params = std::make_shared<ParametrosCompartidos>();
    
    // Inicializar vars->running = true (HiloPID lo usa)
    vars->running = true;
    
    // --- Crear mutex compartido ---
    auto mtx = std::make_shared<pthread_mutex_t>();
    pthread_mutex_init(mtx.get(), nullptr);
    
    // --- Crear running flag compartido ---
    auto running = std::make_shared<bool>(false);  // se activará con interruptor->setRun(1)
    
    //-------------------------------------------------------------
    // ---------------- Frecuencias de muestreo -------------------
    //-------------------------------------------------------------
    const double Ts_controller = 0.01;          // período PID (s)
    const double Ts_component  = Ts_controller / 10.0; // resto de componentes
    const double freq_controller = 1.0 / Ts_controller; // Hz
    const double freq_component  = 1.0 / Ts_component;  // Hz

    // --- Crear InterruptorArranque y HiloIntArranque ---
    auto interruptor = std::make_shared<InterruptorArranque>();

    // Activar running a través del interruptor
    interruptor->setRun(1);

    // Crear el hilo del interruptor de arranque/paro (frecuencia de componentes)
    HiloIntArranque hiloInterruptor(interruptor, running.get(), mtx, freq_component);

    //-------------------------------------------------------------
    // --------- Crear la referencia (SignalSwitch) ---------------
    //-------------------------------------------------------------
    // Parámetros del escalón
    double Ts_signal = Ts_component;     // período de muestreo común [s]
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
    auto signalSwitch = std::make_shared<SignalGenerator::SignalSwitch>(stepSignal, pwmSignal, sinSignal, 0);
    
    // Crear shared_ptr que apunta a vars->ref
    std::shared_ptr<double> ref(&vars->ref, [](double*){});
    
    // Crear HiloSwitch para ejecutar el switch periódicamente (escribe en vars->ref)
    HiloSwitch hiloRef(signalSwitch, ref, running.get(), mtx, params, freq_component);
  
  
    //-------------------------------------------------------------
    // ---------------- Crear la planta ----------------------------
    //---------------------------------------------------------------
    // Planta discretizada a Ts_component = 0.001 s (1 kHz)
    // Planta continua: 1 / (tau*s + 1)
    double tau = 1.0; // constante de tiempo [s]
    std::vector<double> num_s = {1.0};
    std::vector<double> den_s = {tau, 1.0};

    // Discretizar con Tustin al período Ts_component
    auto tf_disc = discretizeTF(num_s, den_s, Ts_component, DiscretizationMethod::Tustin);

    double frequency_plant = freq_component;  // Hz
    size_t bufferSize = 10;

    // Crear sistema discreto con coeficientes discretizados y Ts_component
    auto planta = std::make_shared<TransferFunctionSystem>(tf_disc.b, tf_disc.a, Ts_component, bufferSize);
    
    // Crear shared_ptr que apuntan a vars (sin gestión de memoria)
    std::shared_ptr<double> ua(&vars->ua, [](double*){});
    std::shared_ptr<double> yk(&vars->yk, [](double*){});

    //-------------------------------------------------------------
    // --------------- Crear hilo de la planta --------------------
    //-------------------------------------------------------------
    // Planta lee vars->ua, escribe vars->yk
    Hilo hiloPlanta(planta, ua, yk, running.get(), mtx, frequency_plant);

    //-------------------------------------------------------------
    // ---------------- Crear ADConverter --------------------------
    //-------------------------------------------------------------
    double Ts_converter = Ts_component;  // período de muestreo

    auto ADconverter = std::make_shared<ADConverter>(Ts_converter);
    std::shared_ptr<double> ykd(&vars->ykd, [](double*){});
    // ADConverter lee vars->yk, escribe vars->ykd
    Hilo hiloAD(ADconverter, yk, ykd, running.get(), mtx, freq_component);

    //-------------------------------------------------------------
    // ---------------- Crear PID ----------------------------------
    //-------------------------------------------------------------

    // Inicializar parámetros compartidos con valores por defecto
    pthread_mutex_lock(mtx.get());
    params->kp = 5.0;
    params->ki = 3.0;
    params->kd = 0.7;
    params->setpoint = 1.0;  // igual a la amplitud del escalón
    pthread_mutex_unlock(mtx.get());

    auto pid = std::make_shared<PIDController>(params->kp, params->ki, params->kd, Ts_controller);
    
    // HiloPID lee vars->e, escribe vars->u, actualiza parámetros dinámicamente
    HiloPID hiloPID(pid.get(), vars.get(), params.get(), freq_controller);

    //-------------------------------------------------------------
    // ---------------- Crear DAConverter --------------------------
    //-------------------------------------------------------------
    auto DAconverter = std::make_shared<DAConverter>(Ts_converter);
    std::shared_ptr<double> u(&vars->u, [](double*){});
    // DAConverter lee vars->u, escribe vars->ua
    Hilo hiloDA(DAconverter, u, ua, running.get(), mtx, freq_component); 
  
    //-------------------------------------------------------------
    // ---------------- Crear Sumador ------------------------------
    //-------------------------------------------------------------
    double Ts_sumador = Ts_component;
    auto sumador = std::make_shared<Sumador>(Ts_sumador);
    std::shared_ptr<double> e(&vars->e, [](double*){});
    // Sumador lee vars->ref y vars->ykd, escribe vars->e (error = ref - ykd)
    Hilo2in hiloSumador(sumador, ref, ykd, e, running.get(), mtx, freq_component);

    //-------------------------------------------------------------
    // -------- Crear transmisor para enviar datos via IPC --------
    //-------------------------------------------------------------
    auto transmisor = std::make_shared<Transmisor>(vars.get());
    if (!transmisor->inicializar()) {
        std::cerr << "Error: No se pudo inicializar el Transmisor" << std::endl;
        *running = false;
        return 1;
    }
    std::cout << "Transmisor inicializado correctamente" << std::endl;

    // --- Crear hilo de transmisión a frecuencia de componentes ---
    HiloTransmisor hiloTransmisor(transmisor, running.get(), mtx, freq_component);
    std::cout << "Hilo de transmisión iniciado a " << freq_component << " Hz" << std::endl;

    //-------------------------------------------------------------
    // -------- Crear receptor para recibir parámetros via IPC -----
    //-------------------------------------------------------------
    auto receptor = std::make_shared<Receptor>(params.get());
    if (!receptor->inicializar()) {
        std::cerr << "Error: No se pudo inicializar el Receptor" << std::endl;
        *running = false;
        return 1;
    }
    std::cout << "Receptor inicializado correctamente" << std::endl;

    // --- Crear hilo de recepción a frecuencia de componentes ---
    HiloReceptor hiloReceptor(receptor, running.get(), mtx, freq_component);
    std::cout << "Hilo de recepción iniciado a " << freq_component << " Hz" << std::endl;

    //-------------------------------------------------------------
    // ---- Bucle principal: monitorizar salida en tiempo real ----
    //-------------------------------------------------------------
    
    while(true) {
        // Leer todas las variables compartidas con protección de mutex
        bool running_now;
        double ref_val, u_val, yk_val, e_val;
        double kp_actual, ki_actual, kd_actual, setpoint_actual;
        int signal_type_actual;
        
        pthread_mutex_lock(mtx.get());
        running_now = *running;
        pthread_mutex_unlock(mtx.get());
        
        pthread_mutex_lock(&vars->mtx);
        ref_val = vars->ref;
        u_val = vars->u;
        yk_val = vars->yk;
        e_val = vars->e;
        pthread_mutex_unlock(&vars->mtx);
        
        pthread_mutex_lock(&params->mtx);
        kp_actual = params->kp;
        ki_actual = params->ki;
        kd_actual = params->kd;
        setpoint_actual = params->setpoint;
        signal_type_actual = params->signal_type;
        pthread_mutex_unlock(&params->mtx);
        
        // Permitir salida con Ctrl+C o por límite de iteraciones
        if (!running_now) break;

        // Obtener el número de iteración real del HiloPID
        int k = hiloPID.getIterations();

        std::cout << "k=" << k
                  << " | Ref=" << ref_val
                  << " | e=" << e_val
                  << " | u=" << u_val
                  << " | yk=" << yk_val
                  << " | Kp=" << kp_actual
                  << " | Ki=" << ki_actual
                  << " | Kd=" << kd_actual
                  << " | Setpoint=" << setpoint_actual
                  << " | Signal=" << signal_type_actual << " (0=step,1=sine,2=pwm)"
                  << " | t=" << transmisor->getTiempoTranscurrido() << "s"
                  << std::endl;
        usleep(50000); // 50 ms entre impresiones
    }
    
    // Señalizar a todos los hilos que deben terminar
    pthread_mutex_lock(mtx.get());
    *running = false;
    pthread_mutex_unlock(mtx.get());
    
    pthread_mutex_lock(&vars->mtx);
    vars->running = false;
    pthread_mutex_unlock(&vars->mtx);

   

    //-------------------------------------------------------------
    // ------- Esperar terminación de todos los hilos -------------
    //-------------------------------------------------------------
    pthread_join(hiloRef.getThread(), nullptr);
    pthread_join(hiloPlanta.getThread(), nullptr);
    pthread_join(hiloAD.getThread(), nullptr);
    pthread_join(hiloPID.getThread(), nullptr);
    pthread_join(hiloDA.getThread(), nullptr);
    pthread_join(hiloSumador.getThread(), nullptr);
    pthread_join(hiloTransmisor.getThread(), nullptr);
    pthread_join(hiloReceptor.getThread(), nullptr);

    // Cerrar transmisor y receptor
    transmisor->cerrar();
    receptor->cerrar();
    
    // Destructor mutex
    pthread_mutex_destroy(mtx.get());

    return 0;
}
