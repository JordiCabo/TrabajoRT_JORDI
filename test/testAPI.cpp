#include "SystemAPI.h"
#include <iostream>
#include <unistd.h>

using namespace DiscreteSystems::SystemAPI;

int main() {
	std::cout << "=== Test SystemAPI ===" << std::endl;

	// 1. Crear el sistema completo (todos los hilos, señales, planta, PID, IPC)
	std::cout << "Creando sistema completo..." << std::endl;
	auto lazo = crearSistema();

	// 2. Verificar que la inicialización fue exitosa
	if (!lazo.ok) {
		std::cerr << "Error al crear lazo: " << lazo.error << std::endl;
		return 1;
	}

	std::cout << "Sistema creado correctamente" << std::endl;

	// 3. Iniciar todos los hilos del sistema (PID, planta, señales, IPC)
	std::cout << "Iniciando sistema..." << std::endl;
	iniciarSistema(lazo);

	// 4. Ejecutar el bucle principal del sistema durante 100 iteraciones (~5 segundos)
	//    Esto simula el funcionamiento en tiempo real y muestra la evolución de las variables
	std::cout << "Ejecutando sistema (100 iteraciones = ~5 segundos)..." << std::endl;
	//ejecutarSistema(lazo, false, 100); // Ejecutar por 100 iteraciones
	ejecutarSistema(lazo, true); // Ejecutar indefinidamente hasta detener manualmente

	// 5. Detener todos los hilos y liberar recursos
	std::cout << "\nDeteniendo sistema..." << std::endl;
	detenerSistema(lazo);

	// 6. Fin del test
	return 0;
}

