#include "SystemAPI.h"
#include <unistd.h>
#include <cstdio>
#include <iostream>
#include <signal.h>
#include <fstream>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <sys/stat.h>

namespace DiscreteSystems {
namespace SystemAPI {

/**
 * @brief Lanza la GUI en un proceso hijo en background.
 *
 * Redirige stdout/stderr de la GUI a /dev/null y espera a que se inicialice.
 * @return PID del proceso GUI lanzado, o -1 si falla.
 */
static pid_t lanzarGui() {
	std::cout << "[SystemAPI] Lanzando GUI..." << std::endl;
	
	pid_t gui_pid = fork();
	if (gui_pid == 0) {
		// Proceso hijo: ejecutar GUI
		// Redirigir stdout y stderr de la GUI a /dev/null para no interferir
		freopen("/dev/null", "w", stdout);
		freopen("/dev/null", "w", stderr);
		
		execl("../Gui/gui_app", "gui_app", (char*)NULL);
		// Si exec falla:
		perror("[SystemAPI] Error execl GUI");
		exit(1);
	} else if (gui_pid < 0) {
		std::cerr << "[SystemAPI] Error al hacer fork() para GUI" << std::endl;
		return -1;
	}
	
	// Esperar un poco para que la GUI se inicialice
	sleep(2);
	std::cout << "[SystemAPI] GUI lanzada (PID: " << gui_pid << ")" << std::endl;
	
	return gui_pid;
}

/**
 * @brief Crea el sistema completo de control en tiempo real.
 *
 * Inicializa todos los hilos, señales, planta, PID y comunicación IPC
 * usando la configuración centralizada de SystemConfig.
 *
 * @return LazoHandle con todos los objetos creados y recursos del sistema.
 */
LazoHandle crearSistema() {
	LazoHandle lazo;
	
	//-------------------------------------------------------------
	// --- Crear directorio de logs en raíz del proyecto ---
	//-------------------------------------------------------------
	mkdir("logs", 0755);
	
	//-------------------------------------------------------------
	// --- Redirigir stderr a archivo con timestamp ---
	//-------------------------------------------------------------
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
	
	//-------------------------------------------------------------
	// --- Lanzar GUI ---
	//-------------------------------------------------------------
	lazo.gui_pid = lanzarGui();
	if (lazo.gui_pid < 0) {
		lazo.error = "No se pudo lanzar la GUI";
		return lazo;
	}
	
	//-------------------------------------------------------------
	// --- Usar frecuencias de SystemConfig si no se especifican ---
	//-------------------------------------------------------------
	const double freq_controller = SystemConfig::FREQ_CONTROLLER;
	const double freq_component = SystemConfig::FREQ_COMPONENT;
	const double freq_comm = SystemConfig::FREQ_COMMUNICATION;
	const double Ts_controller = SystemConfig::TS_CONTROLLER;
	const double Ts_component = SystemConfig::TS_COMPONENT;
	const double Ts_signal = Ts_component;
	
	//-------------------------------------------------------------
	// --- Crear variables compartidas ---
	//-------------------------------------------------------------
	lazo.vars = std::make_shared<VariablesCompartidas>();
	lazo.params = std::make_shared<ParametrosCompartidos>();
	lazo.vars->running = true;
	
	//-------------------------------------------------------------
	// --- Crear mutex y running flag ---
	//-------------------------------------------------------------
	lazo.mtx = std::make_shared<pthread_mutex_t>();
	pthread_mutex_init(lazo.mtx.get(), nullptr);
	lazo.running = std::make_shared<bool>(false);
	
	//-------------------------------------------------------------
	// --- Crear InterruptorArranque y su hilo ---
	//-------------------------------------------------------------
	lazo.interruptor = std::make_shared<InterruptorArranque>();
	lazo.interruptor->setRun(1);
	lazo.hiloInterruptor = std::make_unique<HiloIntArranque>(
		lazo.interruptor, lazo.running.get(), lazo.mtx, freq_component, SystemConfig::HILO_INTERRUPT_NAME);
	
	//-------------------------------------------------------------
	// --------- Crear la referencia (SignalSwitch) ---------------
	//-------------------------------------------------------------
	auto stepSignal = std::make_shared<SignalGenerator::StepSignal>(
		Ts_signal, SystemConfig::SIGNAL_AMPLITUDE, SystemConfig::SIGNAL_STEP_TIME, SystemConfig::SIGNAL_OFFSET);
	auto sinSignal = std::make_shared<SignalGenerator::SineSignal>(
		Ts_signal, SystemConfig::SIGNAL_SIN_AMP, SystemConfig::SIGNAL_SIN_FREQ, SystemConfig::SIGNAL_SIN_PHASE, SystemConfig::SIGNAL_OFFSET);
	auto pwmSignal = std::make_shared<SignalGenerator::PwmSignal>(
		Ts_signal, SystemConfig::SIGNAL_AMPLITUDE, SystemConfig::SIGNAL_PWM_DUTY, SystemConfig::SIGNAL_PWM_PERIOD, SystemConfig::SIGNAL_OFFSET);
	auto signalSwitch = std::make_shared<SignalGenerator::SignalSwitch>(
		stepSignal, pwmSignal, sinSignal, SystemConfig::SIGNAL_INITIAL_TYPE);
	
	std::shared_ptr<double> ref(&lazo.vars->ref, [](double*){});
	lazo.hiloRef = std::make_unique<HiloSwitch>(
		signalSwitch, ref, lazo.running.get(), lazo.mtx, lazo.params, freq_component, SystemConfig::HILO_REF_NAME);
	
	//-------------------------------------------------------------
	// --- Crear planta discretizada ---
	//-------------------------------------------------------------
	double tau = SystemConfig::PLANTA_TAU;
	std::vector<double> num_s(SystemConfig::PLANTA_NUM_S, SystemConfig::PLANTA_NUM_S + sizeof(SystemConfig::PLANTA_NUM_S)/sizeof(double));
	std::vector<double> den_s(SystemConfig::PLANTA_DEN_S, SystemConfig::PLANTA_DEN_S + sizeof(SystemConfig::PLANTA_DEN_S)/sizeof(double));
	auto tf_disc = discretizeTF(num_s, den_s, Ts_component, DiscretizationMethod::Tustin);
	lazo.planta = std::make_shared<TransferFunctionSystem>(tf_disc.b, tf_disc.a, Ts_component, 10);
	
	std::shared_ptr<double> ua(&lazo.vars->ua, [](double*){});
	std::shared_ptr<double> yk(&lazo.vars->yk, [](double*){});
	lazo.hiloPlanta = std::make_unique<DiscreteSystems::Hilo>(
		lazo.planta, ua, yk, lazo.running.get(), lazo.mtx, freq_component, SystemConfig::HILO_PLANTA_NAME);
	
	//-------------------------------------------------------------
	// --- Crear ADConverter ---
	//-------------------------------------------------------------
	auto ADconverter = std::make_shared<ADConverter>(Ts_component);
	std::shared_ptr<double> ykd(&lazo.vars->ykd, [](double*){});
	lazo.hiloAD = std::make_unique<DiscreteSystems::Hilo>(
		ADconverter, yk, ykd, lazo.running.get(), lazo.mtx, freq_component, SystemConfig::HILO_AD_NAME);
	
	//-------------------------------------------------------------
	// --- Inicializar parámetros PID ---
	//-------------------------------------------------------------
	pthread_mutex_lock(lazo.mtx.get());
	lazo.params->kp = SystemConfig::PID_KP;
	lazo.params->ki = SystemConfig::PID_KI;
	lazo.params->kd = SystemConfig::PID_KD;
	lazo.params->setpoint = SystemConfig::PID_SETPOINT;
	pthread_mutex_unlock(lazo.mtx.get());
	
	//-------------------------------------------------------------
	// --- Crear PID ---
	//-------------------------------------------------------------
	lazo.pid = std::make_shared<PIDController>(SystemConfig::PID_KP, SystemConfig::PID_KI, SystemConfig::PID_KD, Ts_controller);
	lazo.hiloPID = std::make_unique<DiscreteSystems::HiloPID>(
		lazo.pid.get(), lazo.vars.get(), lazo.params.get(), freq_controller, SystemConfig::HILO_PID_NAME);
	
	//-------------------------------------------------------------
	// --- Crear DAConverter ---
	//-------------------------------------------------------------
	auto DAconverter = std::make_shared<DAConverter>(Ts_component);
	std::shared_ptr<double> u(&lazo.vars->u, [](double*){});
	lazo.hiloDA = std::make_unique<DiscreteSystems::Hilo>(
		DAconverter, u, ua, lazo.running.get(), lazo.mtx, freq_component, SystemConfig::HILO_DA_NAME);
	
	//-------------------------------------------------------------
	// --- Crear Sumador ---
	//-------------------------------------------------------------
	auto sumador = std::make_shared<Sumador>(Ts_component);
	std::shared_ptr<double> e(&lazo.vars->e, [](double*){});
	lazo.hiloSumador = std::make_unique<DiscreteSystems::Hilo2in>(
		sumador, ref, ykd, e, lazo.running.get(), lazo.mtx, freq_component, SystemConfig::HILO_SUMADOR_NAME);
	
	//-------------------------------------------------------------
	// --- Crear Transmisor ---
	//-------------------------------------------------------------
	lazo.transmisor = std::make_shared<Transmisor>(lazo.vars.get());
	if (!lazo.transmisor->inicializar()) {
		lazo.error = "No se pudo inicializar el Transmisor";
		return lazo;
	}
	lazo.hiloTransmisor = std::make_unique<HiloTransmisor>(
		lazo.transmisor, lazo.running.get(), lazo.mtx, freq_comm);
	
	//-------------------------------------------------------------
	// --- Crear Receptor ---
	//-------------------------------------------------------------
	lazo.receptor = std::make_shared<Receptor>(lazo.params.get());
	if (!lazo.receptor->inicializar()) {
		lazo.error = "No se pudo inicializar el Receptor";
		return lazo;
	}
	lazo.hiloReceptor = std::make_unique<HiloReceptor>(
		lazo.receptor, lazo.running.get(), lazo.mtx, freq_comm);
	
	lazo.ok = true;
	return lazo;
}

/**
 * @brief Inicia la ejecución de todos los hilos del sistema.
 *
 * @param lazo Handle del sistema creado por crearSistema().
 */
void iniciarSistema(LazoHandle& lazo) {
	if (!lazo.ok) return;
	
	pthread_mutex_lock(lazo.mtx.get());
	*lazo.running = true;
	pthread_mutex_unlock(lazo.mtx.get());
	
	pthread_mutex_lock(&lazo.vars->mtx);
	lazo.vars->running = true;
	pthread_mutex_unlock(&lazo.vars->mtx);
	
	if (lazo.interruptor) {
		lazo.interruptor->setRun(1);
	}
}

/**
 * @brief Detiene todos los hilos y libera recursos del sistema.
 *
 * @param lazo Handle del sistema
 */
void detenerSistema(LazoHandle& lazo) {
	if (!lazo.ok) return;
	
	// Señalizar a todos los hilos que deben terminar
	pthread_mutex_lock(lazo.mtx.get());
	*lazo.running = false;
	pthread_mutex_unlock(lazo.mtx.get());
	
	pthread_mutex_lock(&lazo.vars->mtx);
	lazo.vars->running = false;
	pthread_mutex_unlock(&lazo.vars->mtx);
	
	// Esperar un poco para que los threads lean running=false y terminen
	// Los destructores se encargarán de pthread_join() y escribir buffers
	sleep(1);
	
	// Destructor mutex
	pthread_mutex_destroy(lazo.mtx.get());
	
	// Matar la GUI
	if (lazo.gui_pid > 0) {
		std::cout << "[SystemAPI] Matando GUI (PID: " << lazo.gui_pid << ")" << std::endl;
		kill(lazo.gui_pid, SIGTERM);
	}
}

/**
 * @brief Ejecuta el bucle de monitorización del sistema (equivalente al while de testSystem).
 *
 * Lee todas las variables del sistema, imprime por consola el estado actual,
 * hace sleep de 50ms, y comprueba si debe continuar ejecutando.
 *
 * @param lazo Handle del sistema
 * @param infinito Si true, ejecuta indefinidamente hasta que se detenga el sistema. Si false, ejecuta un número fijo de iteraciones
 * @param max_iteraciones Número máximo de iteraciones (solo se usa si infinito=false)
 */
void ejecutarSistema(LazoHandle& lazo, bool infinito, int max_iteraciones) {
	if (!lazo.ok) return;
	
	// Limpiar buffer si es ejecución finita
	if (!infinito && max_iteraciones > 0) {
		lazo.vars->limpiarBuffer();
	}
	
	int iteracion = 0;
	
	while(true) {
		// Leer estado de ejecución
		bool running_now;
		
		pthread_mutex_lock(lazo.mtx.get());
		running_now = *lazo.running;
		pthread_mutex_unlock(lazo.mtx.get());
		
		// Permitir salida con Ctrl+C o por detección de running=false
		if (!running_now) break;
		
		// Proteger lectura de variables compartidas
		pthread_mutex_lock(&lazo.vars->mtx);
		pthread_mutex_unlock(&lazo.vars->mtx);
		
		// Obtener el número de iteración real del HiloPID
		int k = 0;
		if (lazo.hiloPID) {
			k = lazo.hiloPID->getIterations();
		}
		
		// Construir línea de salida
		std::ostringstream linea;
		linea << "k=" << k << " | " << *lazo.vars;
		std::string salida = linea.str();
		
		// Imprimir por consola
		std::cout << salida << std::endl;
		
		// Guardar en buffer si es ejecución finita (O(1), real-time safe)
		if (!infinito && max_iteraciones > 0) {
			lazo.vars->guardarMuestraEnBuffer(k, salida);
		}
		
		// 50 ms entre impresiones
		usleep(50000);
		
		// Control de iteraciones
		if (!infinito) {
			iteracion++;
			if (iteracion >= max_iteraciones) {
				// Señalizar a través del interruptor que debe detener
				if (lazo.interruptor) {
					lazo.interruptor->setRun(0);
				}
				break;
			}
		}
	}
}

} // namespace SystemAPI
} // namespace DiscreteSystems
