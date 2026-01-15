#include <iostream>
#include <unistd.h>
#include <mutex>
#include <memory>
#include <cstdio>
#include <ctime>
#include <csignal>
#include <sstream>
#include <iomanip>
#include <sys/stat.h>
#include "config/system_config.h"
#include "sistemas/TransferFunctionSystem.h"
#include "hilos/Hilo.h"
#include "hilos/Hilo2in.h"
#include "hilos/HiloPID.h"
#include "utilidades/VariablesCompartidas.h"
#include "utilidades/ParametrosCompartidos.h"
#include "sistemas/PIDController.h"
#include "sistemas/DiscreteSystem.h"
#include <pthread.h>
#include "converters/ADConverter.h"
#include "utilidades/Sumador.h"
#include "converters/DAConverter.h"
#include "hilos/HiloSignal.h"
#include "senales/SignalSwitch.h"
#include "hilos/HiloSwitch.h"
#include "io/Transmisor.h"
#include "hilos/HiloTransmisor.h"
#include "io/Receptor.h"
#include "hilos/HiloReceptor.h"
#include "utilidades/InterruptorArranque.h"
#include "hilos/HiloIntArranque.h"
#include "sistemas/Discretizer.h"

using namespace DiscreteSystems;

// Variables globales para signal handler
static bool* g_running_ptr = nullptr;
static VariablesCompartidas* g_vars_ptr = nullptr;

// Signal handler para Ctrl+C (SIGINT)
void signalHandler(int signum) {
    std::cerr << "\n[Signal Handler] Capturado señal " << signum << " (Ctrl+C)" << std::endl;
    std::cerr << "[Signal Handler] Deteniendo hilos de forma limpia..." << std::endl;
    
    if (g_running_ptr) {
        *g_running_ptr = false;
    }
    if (g_vars_ptr) {
        pthread_mutex_lock(&g_vars_ptr->mtx);
        g_vars_ptr->running = false;
        pthread_mutex_unlock(&g_vars_ptr->mtx);
    }
}

// Función para lanzar la GUI en background
pid_t lanzarGui() {
    std::cout << "[Main] Lanzando GUI..." << std::endl;
    
    pid_t gui_pid = fork();
    if (gui_pid == 0) {
        // Proceso hijo: ejecutar GUI
        // Redirigir stdout y stderr de la GUI a /dev/null para no interferir
        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "w", stderr);
        
        execl("../Gui/gui_app", "gui_app", (char*)NULL);
        // Si exec falla (se escribe antes de redirigir stderr del proceso principal):
        perror("[Main] Error execl GUI");
        exit(1);
    } else if (gui_pid < 0) {
        std::cerr << "[Main] Error al hacer fork() para GUI" << std::endl;
        return -1;
    }
    
    // Esperar un poco para que la GUI se inicialice
    sleep(2);
    std::cout << "[Main] GUI lanzada (PID: " << gui_pid << ")" << std::endl;
    
    return gui_pid;
}

int main() {
    // --- Lanzar GUI ---
    pid_t gui_pid = lanzarGui();
    if (gui_pid < 0) {
        return 1;
    }
    
    // --- Crear directorio de logs en raíz del proyecto ---
    mkdir("../logs", 0755);
    
    // --- Redirigir stderr a archivo con timestamp ---
    time_t now = time(nullptr);
    struct tm* tm_info = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", tm_info);
    
    std::ostringstream error_file;
    error_file << "../logs/error_log_" << timestamp << ".txt";
    
    FILE* error_log = freopen(error_file.str().c_str(), "w", stderr);
    if (error_log) {
        setbuf(stderr, NULL);  // Unbuffered para flush inmediato
        std::cerr << "=== Error Log Started ===" << std::endl;
        std::cerr << "Timestamp: " << timestamp << std::endl;
        std::cerr << "=========================" << std::endl << std::endl;
    }
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
    
    // --- Registrar signal handler para Ctrl+C ---
    g_running_ptr = running.get();
    g_vars_ptr = vars.get();
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    std::cerr << "[Main] Signal handlers registrados (Ctrl+C para parada limpia)" << std::endl;
    
    //-------------------------------------------------------------
    // ---------------- Frecuencias de muestreo -------------------
    //-------------------------------------------------------------
    // Usar configuración centralizada desde system_config.h
    const double Ts_controller = SystemConfig::TS_CONTROLLER;
    const double Ts_component  = SystemConfig::TS_COMPONENT;
    const double freq_controller = SystemConfig::FREQ_CONTROLLER;
    const double freq_component  = SystemConfig::FREQ_COMPONENT;
    const double freq_communication = SystemConfig::FREQ_COMMUNICATION;

    // --- Crear InterruptorArranque y HiloIntArranque ---
    auto interruptor = std::make_shared<InterruptorArranque>();

    // Activar running a través del interruptor
    interruptor->setRun(1);

    // Crear el hilo del interruptor de arranque/paro (frecuencia de componentes)
    HiloIntArranque hiloInterruptor(interruptor, running.get(), mtx, freq_component, SystemConfig::HILO_INTERRUPT_NAME);

    //-------------------------------------------------------------
    // --------- Crear la referencia (SignalSwitch) ---------------
    //-------------------------------------------------------------
    // Parámetros del escalón y señales desde SystemConfig
    double Ts_signal = Ts_component;     // período de muestreo común [s]
    double amplitude   = SystemConfig::SIGNAL_AMPLITUDE;
    double step_time   = SystemConfig::SIGNAL_STEP_TIME;
    double offset      = SystemConfig::SIGNAL_OFFSET;

    // Parámetros para el seno
    double freq        = SystemConfig::SIGNAL_SIN_FREQ;
    double phase       = SystemConfig::SIGNAL_SIN_PHASE;
    double sinAmp      = SystemConfig::SIGNAL_SIN_AMP;

    // Parámetros para PWM
    double duty        = SystemConfig::SIGNAL_PWM_DUTY;
    double period_pwm  = SystemConfig::SIGNAL_PWM_PERIOD;

    // Crear las 3 señales como shared_ptr
    auto stepSignal = std::make_shared<SignalGenerator::StepSignal>(Ts_signal, amplitude, step_time, offset);
    auto sinSignal = std::make_shared<SignalGenerator::SineSignal>(Ts_signal, sinAmp, freq, phase, offset);
    auto pwmSignal = std::make_shared<SignalGenerator::PwmSignal>(Ts_signal, amplitude, duty, period_pwm, offset);

    // Crear SignalSwitch: 0=step, 1=pwm, 2=sine (selector inicial = 0: escalón)
    auto signalSwitch = std::make_shared<SignalGenerator::SignalSwitch>(stepSignal, pwmSignal, sinSignal, 0);
    
    // Crear shared_ptr que apunta a vars->ref
    std::shared_ptr<double> ref(&vars->ref, [](double*){});
    
    // Crear HiloSwitch para ejecutar el switch periódicamente (escribe en vars->ref)
    HiloSwitch hiloRef(signalSwitch, ref, running.get(), mtx, params, freq_component, SystemConfig::HILO_REF_NAME);
  
  
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
    Hilo hiloPlanta(planta, ua, yk, running.get(), mtx, frequency_plant, SystemConfig::HILO_PLANTA_NAME);

    //-------------------------------------------------------------
    // ---------------- Crear ADConverter --------------------------
    //-------------------------------------------------------------
    double Ts_converter = Ts_component;  // período de muestreo

    auto ADconverter = std::make_shared<ADConverter>(Ts_converter);
    std::shared_ptr<double> ykd(&vars->ykd, [](double*){});
    // ADConverter lee vars->yk, escribe vars->ykd
    Hilo hiloAD(ADconverter, yk, ykd, running.get(), mtx, freq_component, SystemConfig::HILO_AD_NAME);

    //-------------------------------------------------------------
    // ---------------- Crear PID ----------------------------------
    //-------------------------------------------------------------

    // Inicializar parámetros compartidos con valores por defecto
    pthread_mutex_lock(mtx.get());
    params->kp = SystemConfig::PID_KP;
    params->ki = SystemConfig::PID_KI;
    params->kd = SystemConfig::PID_KD;
    params->setpoint = SystemConfig::PID_SETPOINT;
    pthread_mutex_unlock(mtx.get());

    auto pid = std::make_shared<PIDController>(params->kp, params->ki, params->kd, Ts_controller);
    
    // HiloPID lee vars->e, escribe vars->u, actualiza parámetros dinámicamente
    HiloPID hiloPID(pid.get(), vars.get(), params.get(), freq_controller, SystemConfig::HILO_PID_NAME);

    //-------------------------------------------------------------
    // ---------------- Crear DAConverter --------------------------
    //-------------------------------------------------------------
    auto DAconverter = std::make_shared<DAConverter>(Ts_converter);
    std::shared_ptr<double> u(&vars->u, [](double*){});
    // DAConverter lee vars->u, escribe vars->ua
    Hilo hiloDA(DAconverter, u, ua, running.get(), mtx, freq_component, SystemConfig::HILO_DA_NAME); 
  
    //-------------------------------------------------------------
    // ---------------- Crear Sumador ------------------------------
    //-------------------------------------------------------------
    double Ts_sumador = Ts_component;
    auto sumador = std::make_shared<Sumador>(Ts_sumador);
    std::shared_ptr<double> e(&vars->e, [](double*){});
    // Sumador lee vars->ref y vars->ykd, escribe vars->e (error = ref - ykd)
    Hilo2in hiloSumador(sumador, ref, ykd, e, running.get(), mtx, freq_component, SystemConfig::HILO_SUMADOR_NAME);

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

    // --- Crear hilo de transmisión a frecuencia de comunicación (2 Hz = 500ms) ---
    HiloTransmisor hiloTransmisor(transmisor, running.get(), mtx, freq_communication, SystemConfig::HILO_TRANSMISOR_NAME);
    std::cout << "Hilo de transmisión iniciado a " << freq_communication << " Hz (500ms)" << std::endl;

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

    // --- Crear hilo de recepción a frecuencia de comunicación (2 Hz = 500ms) ---
    HiloReceptor hiloReceptor(receptor, running.get(), mtx, freq_communication, SystemConfig::HILO_RECEPTOR_NAME);
    std::cout << "Hilo de recepción iniciado a " << freq_communication << " Hz (500ms)" << std::endl;

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

    // Esperar un poco para que los threads lean running=false y terminen
    // Los destructores se encargarán de pthread_join()
    sleep(1);

    //-------------------------------------------------------------
    // ------- Destructores de threads: cleanup automático --------
    //-------------------------------------------------------------
    // IMPORTANTE: Los destructores de Hilo, HiloSignal, etc. ya hacen
    // pthread_join() automáticamente, por lo que NO hacer join manual aquí.
    // Los threads se limpian automáticamente cuando los objetos salen del scope.
    //
    // Si hiciéramos pthread_join() aquí Y en el destructor, sería double-join
    // que causa segmentation fault.
    
    // Transmisor y receptor se cierran automáticamente en sus destructores
    // (no llamar cerrar() manualmente para evitar duplicación de mensajes)
    
    // Destructor mutex
    pthread_mutex_destroy(mtx.get());

    return 0;
}
