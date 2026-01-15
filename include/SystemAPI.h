// SystemAPI: Funciones helper para crear lazos de control sin repetir código de testSystem.cpp
#pragma once

#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <pthread.h>

// Incluir TODOS los headers necesarios (sin forward declarations)
#include "config/system_config.h"
#include "sistemas/TransferFunctionSystem.h"
#include "sistemas/PIDController.h"
#include "sistemas/Discretizer.h"
#include "hilos/Hilo.h"
#include "hilos/Hilo2in.h"
#include "hilos/HiloPID.h"
#include "hilos/HiloSwitch.h"
#include "hilos/HiloTransmisor.h"
#include "hilos/HiloReceptor.h"
#include "hilos/HiloIntArranque.h"
#include "converters/ADConverter.h"
#include "converters/DAConverter.h"
#include "utilidades/Sumador.h"
#include "utilidades/InterruptorArranque.h"
#include "utilidades/VariablesCompartidas.h"
#include "utilidades/ParametrosCompartidos.h"
#include "io/Transmisor.h"
#include "io/Receptor.h"
#include "senales/SignalSwitch.h"
#include "senales/SignalGenerator.h"

namespace DiscreteSystems {
namespace SystemAPI {

/**
 * @brief Estructura que contiene todas las opciones para configurar el lazo de control
 */

/**
 * @brief Estructura que contiene todos los objetos del lazo de control creado
 */
/**
 * @brief Handle que agrupa todos los objetos y recursos del lazo de control creado.
 * 
 * Contiene punteros a variables compartidas, parámetros, mutex, hilos, sistemas discretos,
 * transmisor/receptor IPC y estado de la GUI. Los hilos se destruyen automáticamente al salir de scope.
 * El campo ok indica si la inicialización fue exitosa; error contiene el mensaje en caso de fallo.
 */
struct LazoHandle {
	/** Variables compartidas entre hilos (referencias, salidas, etc.) */
	std::shared_ptr<VariablesCompartidas> vars;
	/** Parámetros compartidos (PID, setpoint, etc.) */
	std::shared_ptr<ParametrosCompartidos> params;
	/** Mutex global para sincronización */
	std::shared_ptr<pthread_mutex_t> mtx;
	/** Flag de ejecución global */
	std::shared_ptr<bool> running;
	/** PID del proceso GUI lanzado */
	pid_t gui_pid{-1};

	// Objetos del lazo
	std::shared_ptr<InterruptorArranque> interruptor;
	std::shared_ptr<TransferFunctionSystem> planta;
	std::shared_ptr<PIDController> pid;
	std::shared_ptr<Transmisor> transmisor;
	std::shared_ptr<Receptor> receptor;

	// Hilos (se destruyen automáticamente al salir de scope)
	std::unique_ptr<HiloIntArranque> hiloInterruptor;
	std::unique_ptr<HiloSwitch> hiloRef;
	std::unique_ptr<DiscreteSystems::Hilo> hiloPlanta;
	std::unique_ptr<DiscreteSystems::Hilo> hiloAD;
	std::unique_ptr<DiscreteSystems::HiloPID> hiloPID;
	std::unique_ptr<DiscreteSystems::Hilo> hiloDA;
	std::unique_ptr<DiscreteSystems::Hilo2in> hiloSumador;
	std::unique_ptr<HiloTransmisor> hiloTransmisor;
	std::unique_ptr<HiloReceptor> hiloReceptor;

	/** True si la inicialización fue exitosa */
	bool ok{false};
	/** Mensaje de error si ok=false */
	std::string error;
};

/**
 * @brief Crea el sistema completo de control en tiempo real.
 *
 * Inicializa todos los hilos, señales, planta, PID y comunicación IPC
 * usando la configuración centralizada de SystemConfig.
 *
 * @return LazoHandle con todos los objetos creados y recursos del sistema.
 */
LazoHandle crearSistema();

/**
 * @brief Inicia la ejecución de todos los hilos del sistema.
 *
 * @param lazo Handle del sistema creado por crearSistema().
 */
void iniciarSistema(LazoHandle& lazo);

/**
 * @brief Detiene todos los hilos y libera recursos del sistema.
 *
 * @param lazo Handle del sistema
 */
void detenerSistema(LazoHandle& lazo);

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
void ejecutarSistema(LazoHandle& lazo, bool infinito = true, int max_iteraciones = 0);

} // namespace SystemAPI
} // namespace DiscreteSystems
