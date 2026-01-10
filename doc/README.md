# PL7 - Control de Sistemas Discretos

[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Documentation](https://img.shields.io/badge/docs-Doxygen-brightgreen.svg)](doc/doxygen/html/index.html)

## 📋 Descripción

Framework educativo de control de sistemas en tiempo real implementado en C++17. Proporciona una librería de sistemas discretos reutilizables (PID, funciones de transferencia, generadores de señal).

**Trabajo Final** para la asignatura de Sistemas en Tiempo Real.

### Características Principales

- 🎛️ **Controladores PID discretos** con sintonización en línea
- 📊 **Sistemas en espacio de estados** y funciones de transferencia
- 📐 **Discretizador continuo→discreto** por Tustin (bilineal) con `Discretizer`
- 📡 **Generadores de señal** (escalón, rampa, senoidal, PWM)
- 🧵 **Ejecución multihilo** con temporización absoluta (`Temporizador` + `clock_nanosleep`)
- 🔄 **Convertidores A/D y D/A** simulados

## 🏗️ Arquitectura

```
PL7/
├── include/              # Headers de la librería core
│   ├── DiscreteSystem.h  # Clase base abstracta (patrón NVI)
│   ├── PIDController.h   # Controlador PID discreto
│   ├── TransferFunctionSystem.h
│   ├── StateSpaceSystem.h
│   ├── SignalGenerator.h # Generadores de señal
│   ├── Hilo.h           # Wrapper de threading
│   └── ...
├── src/                  # Implementaciones (.cpp)
├── test/                 # Tests unitarios (auto-descubiertos)
├── Interfaz_Control/     # Interfaz de control (separada)
│   ├── src/             # Código fuente
│   ├── include/         # Headers
│   └── bin/             # Ejecutables
├── doc/                  # Documentación generada
│   └── doxygen/         # Documentación HTML
└── CMakeLists.txt        # Build system raíz
```

### Componentes

#### 1. Librería Core (`src/`, `include/`)
Sistemas discretos C++17 reutilizables:
- **DiscreteSystem**: Clase base con patrón NVI y buffer circular
- **PIDController**: Control PID discreto con ecuación en diferencias
- **TransferFunctionSystem**: Sistemas SISO con función de transferencia
- **StateSpaceSystem**: Representación en espacio de estados
- **SignalGenerator**: Señales de prueba (step, sine, ramp, PWM)
- **Discretizer**: Bilineal (Tustin) de B(s)/A(s) a B(z)/A(z)
- **Temporizador**: Temporización absoluta sobre `CLOCK_MONOTONIC`
- **Hilo/Hilo2in/HiloSignal**: Ejecución pthread a frecuencia fija

#### 2. Componentes IPC y Comunicación
Sistema de comunicación entre procesos para GUI en tiempo real:
- **Receptor**: Recibe parámetros PID desde mqueue (GUI → Simulador)
- **Transmisor**: Envía datos de control para visualización (Simulador → GUI)
- **ParametrosCompartidos**: Variables thread-safe para Kp, Ki, Kd, setpoint
- **VariablesCompartidas**: Variables thread-safe del lazo de control (ref, e, u, y, yk)
- **Serialización manual**: Sin padding de structs para portabilidad

#### 3. Hilos Especializados
Wrappers de threading para componentes IPC:
- **HiloPID**: Ejecutor especializado de PIDController con parámetros dinámicos
- **HiloReceptor**: Recepción periódica de parámetros desde GUI
- **HiloTransmisor**: Envío periódico de datos de control a GUI
- **HiloSwitch**: Multiplexado dinámico de señales de referencia
- **HiloSignal**: Generación periódica de señal de referencia
- **Hilo/Hilo2in**: Ejecutores generales para cualquier DiscreteSystem

#### 4. Componentes Auxiliares (`Interfaz_Control/`)
Proyecto separado de demostración:
- **control_simulator**: Ejecutable que corre el lazo de control con IPC
- **gui_app**: Interfaz Qt6 para visualización y sintonización en vivo
- **test_send/test_receive**: Utilidades para probar comunicación IPC

## 🚀 Compilación

### Requisitos

- **Compilador**: GCC/Clang con soporte C++17
- **CMake**: >= 3.10
- **pthread**: Soporte POSIX threads
- **rt**: Extensiones de tiempo real (message queues)
- **Doxygen** (opcional): Para documentación
- **Graphviz** (opcional): Para diagramas

### Instalación de Dependencias

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake doxygen graphviz
```

**Arch Linux:**
```bash
sudo pacman -S base-devel cmake doxygen graphviz
```

### Build Completo

```bash
# Clonar el repositorio
cd /home/jordi/PLs/PL7

# Compilar librería core
mkdir -p build && cd build
cmake ..
make
cd ..
```

### Build Manual por Partes

**Librería Core:**
```bash
cd build
cmake ..
make
```

## 🧪 Testing

Los tests se auto-descubren desde el directorio `test/`. Cada archivo `.cpp` genera un ejecutable:

```bash
# Ejecutar test individual
./bin/testPID
./bin/testTF
./bin/testStepSignal

# Ver muestras generadas
ls test/*.csv test/*.tsv
```

## 🎮 Uso

### Ejemplo de Código: PID Simple

```cpp
#include "PIDController.h"
#include "TransferFunctionSystem.h"

int main() {
    double Ts = 0.001;  // 1ms de muestreo
    
    // Sistema de primer orden: G(s) = 1/(0.3s + 1)
    std::vector<double> num = {1.0};
    std::vector<double> den = {0.3, 1.0};
    DiscreteSystems::TransferFunctionSystem planta(num, den, Ts);
    
    // Controlador PID
    DiscreteSystems::PIDController pid(1.0, 0.5, 0.1, Ts);
    
    // Simulación en bucle cerrado
    double setpoint = 1.0;
    for (int k = 0; k < 1000; k++) {
        double y = planta.compute();
        double error = setpoint - y;
        double u = pid.next(error);
        planta.next(u);
    }
    
    return 0;
}
```

### Ejemplo: Sistema Completo con GUI en Tiempo Real

```cpp
// control_simulator.cpp - Lazo de control con IPC

#include "HiloPID.h"
#include "HiloSwitch.h"
#include "HiloReceptor.h"
#include "HiloTransmisor.h"
#include "SignalGenerator.h"

int main() {
    // Estructuras compartidas (thread-safe)
    ParametrosCompartidos params;   // Recibe Kp, Ki, Kd de GUI
    VariablesCompartidas vars;      // Estado del lazo (ref, e, u, yk)
    
    // Componentes de control
    auto step = std::make_shared<SignalGenerator::StepSignal>(0.001, 1.0);
    auto sine = std::make_shared<SignalGenerator::SineSignal>(0.001, 1.0, 0.5);
    SignalGenerator::SignalSwitch sw(step, sine, 1);
    
    DiscreteSystems::PIDController pid(1.0, 0.5, 0.1, 0.001);
    DiscreteSystems::TransferFunctionSystem planta(/*...*/, 0.001);
    
    // Comunicación IPC
    Receptor receptor(&params);
    Transmisor transmisor(&vars);
    
    if (receptor.inicializar() && transmisor.inicializar()) {
        // Crear hilos especializados
        HiloSwitch hiloSw(&sw, &vars.ref, &vars.running, &vars.mtx, &params, 100);
        HiloPID hiloPID(&pid, &vars, &params, 100);
        Hilo hiloSumador(&sumador, &vars.ref, &vars.e, &vars.running, &vars.mtx, 100);
        HiloReceptor hiloRx(&receptor, &vars.running, &vars.mtx, 50);
        HiloTransmisor hiloTx(&transmisor, &vars.running, &vars.mtx, 50);
        
        // El sistema está ejecutando automáticamente...
        sleep(10);  // Simular 10 segundos
        
        // Señal de detención
        {
            std::lock_guard<pthread_mutex_t> lock(vars.mtx);
            vars.running = false;
        }
    }
    
    transmisor.cerrar();
    receptor.cerrar();
    return 0;
}
```

Esta arquitectura permite:
1. **Ejecución en tiempo real**: Lazo de control a frecuencia fija (~1 kHz)
2. **Visualización en vivo**: GUI recibe datos a 50 Hz sin afectar al lazo
3. **Sintonización dinámica**: Cambiar Kp, Ki, Kd en tiempo real desde GUI
4. **Multiplexado de señales**: Cambiar entre escalón/rampa/senoidal sin interrumpir

## 📚 Documentación

### Generar Documentación Doxygen

```bash
doxygen Doxyfile
xdg-open doc/doxygen/html/index.html
```

### Documentos Adicionales

- [Instrucciones Copilot](.github/copilot-instructions.md) - Guía para agentes IA

## 🔧 Configuración

### Parámetros de Compilación

Editar `CMakeLists.txt`:
```cmake
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3")
```

## 🐛 Troubleshooting

### Error: "could not open lock file"
Necesitas permisos sudo para instalar dependencias.

### Error: "cannot create /queue"
Las colas POSIX requieren permisos. Verifica:
```bash
ls -la /dev/mqueue/
```

### Tests fallan
Limpia y recompila:
```bash
rm -rf build/ Interfaz_Control/build/
# Recompilar desde cero
```

## 📖 Conceptos Clave

### Patrón NVI (Non-Virtual Interface)
`DiscreteSystem::next()` es público y no-virtual; garantiza almacenamiento en buffer. Las subclases sobrescriben `compute()` protegido.

### Buffer Circular
Evita asignaciones dinámicas en el hot loop. Implementado con `std::deque` e índices manuales.

### IPC con Serialización Manual
Structs sin padding para portabilidad entre procesos. Uso de `serializeDataMessage()`.

### Threading de Frecuencia Fija
`Hilo` usa `usleep()` para mantener período constante de ejecución.

## 👥 Autoría

- **Autor**: Jordi
- **Asistencia**: GitHub Copilot
- **Proyecto**: Trabajo Final - Sistemas en Tiempo Real
- **Versión**: Diciembre 2024

## 📄 Licencia

Este proyecto es material educativo. Consulta el archivo [LICENSE](LICENSE) para más detalles.

## 🤝 Contribuciones

Este es un proyecto educativo. Para contribuir:

1. Haz fork del repositorio
2. Crea una rama para tu feature (`git checkout -b feature/nueva-funcionalidad`)
3. Commit tus cambios (`git commit -am 'Añadir nueva funcionalidad'`)
4. Push a la rama (`git push origin feature/nueva-funcionalidad`)
5. Crea un Pull Request

Consulta [CONTRIBUTING.md](CONTRIBUTING.md) para más detalles.

## 📞 Soporte

Para preguntas o problemas:
- Abre un issue en el repositorio
- Consulta la documentación Doxygen
- Revisa los ejemplos en `test/`

---

**Nota**: Este proyecto es un trabajo final académico que demuestra la implementación de principios de control en tiempo real y aplicación de mejores prácticas de C++17 con asistencia de GitHub Copilot.
