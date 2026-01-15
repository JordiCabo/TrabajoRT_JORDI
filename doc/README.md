
# PL7 - Control de Sistemas Discretos

[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Documentation](https://img.shields.io/badge/docs-Doxygen-brightgreen.svg)](doc/doxygen/html/index.html)

## 📋 Descripción

Framework educativo de control de sistemas en tiempo real implementado en C++17. Proporciona una librería de sistemas discretos reutilizables (PID, funciones de transferencia, generadores de señal).

**Trabajo Final** para la asignatura de Sistemas en Tiempo Real.
**Versión:** Enero 2026

## 🏗️ Arquitectura

### Estructura de Carpetas

| Carpeta/Archivo         | Propósito principal                                      |
|------------------------|----------------------------------------------------------|
| include/               | Headers de la librería core (sistemas, señales, hilos)   |
| src/                   | Implementaciones (.cpp)                                  |
| test/                  | Tests unitarios auto-descubiertos                        |
| Gui/                   | GUI Qt6 (binario gui_app)                                |
| bin/                   | Binarios generados tras la compilación (gui_app, tests)  |
| doc/                   | Documentación, diagramas, Doxygen                        |
| CMakeLists.txt         | Configuración de build y tests                           |

## 🚦 Primeros Pasos tras la Instalación

1. Compila el proyecto:
    ```bash
    cd build
    cmake .. && make
    ```
2. Los binarios se generan en la carpeta `bin/` (por ejemplo, `./bin/gui_app`, `./bin/testPID`).
3. Ejecuta un test básico para verificar la instalación:
    ```bash
    ./bin/testPID
    ```
    Si ves resultados numéricos o logs, la instalación es correcta.
4. Para lanzar la interfaz gráfica:
    ```bash
    ./bin/gui_app
    ```
5. Consulta la documentación en `doc/` y la API Doxygen en `doc/doxygen/html/index.html`.


### Características Principales

- 🎛️ **Controladores PID discretos** con sintonización en línea y timedlock (timeout 20%)
- 📊 **Sistemas en espacio de estados** y funciones de transferencia
- 📐 **Discretizador continuo→discreto** por Tustin (bilineal) con `Discretizer`
- 📡 **Generadores de señal** (StepSignal, SineSignal, RampSignal, PWMSignal, SignalMixer)
- 🧵 **Ejecución multihilo** con temporización absoluta (`Temporizador` + `clock_nanosleep`)
- 🔄 **Convertidores A/D y D/A** simulados
- 📝 **RuntimeLogger** con buffer circular para diagnóstico en tiempo real (solo hilos de control, logging selectivo)
- 🔒 **Signal handler** (SIGINT/SIGTERM) para parada limpia de hilos
- ⚙️ **Configuración centralizada** (SSOT, `constexpr`) en `system_config.h`

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
├── Gui/                  # Interfaz gráfica Qt6 (gui_app)
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

#### 4. Componentes Auxiliares (GUI y binarios)
Componentes de demostración y utilidades:
- **gui_app**: Interfaz Qt6 para visualización y sintonización en vivo (en `Gui/` y `bin/`)
- **test_send/test_receive**: Utilidades para probar comunicación IPC (en `bin/`)

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

Los tests se auto-descubren desde el directorio `test/`. Cada archivo `.cpp` genera un ejecutable en `./bin/`.
Estructura modular: tests para PID, señales, IPC, etc. Los resultados de los tests suelen guardarse en archivos `.csv` o `.tsv` dentro de `test/`.

```bash
# Ejecutar test individual
./bin/testPID
./bin/testTF
./bin/testStepSignal

# Ver muestras generadas por los tests
ls test/*.csv test/*.tsv
```

## 🎮 Uso


### Ejemplo de Código: PID Simple

```cpp
#include "sistemas/PIDController.h"
#include "sistemas/TransferFunctionSystem.h"

int main() {
    double Ts = 0.001;  // Periodo de muestreo: 1 ms
    // Definir sistema de primer orden: G(s) = 1/(0.3s + 1)
    std::vector<double> num = {1.0};
    std::vector<double> den = {0.3, 1.0};
    DiscreteSystems::TransferFunctionSystem planta(num, den, Ts);
    // Crear controlador PID discreto
    DiscreteSystems::PIDController pid(1.0, 0.5, 0.1, Ts);
    // Simulación en lazo cerrado
    double setpoint = 1.0;
    for (int k = 0; k < 1000; k++) {
        double y = planta.compute(); // Salida actual
        double error = setpoint - y; // Error de referencia
        double u = pid.next(error);  // Señal de control
        planta.next(u);              // Aplicar control a la planta
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
    ParametrosCompartidos params;   // Recibe Kp, Ki, Kd, setpoint, signal_type de GUI
    VariablesCompartidas vars;      // Estado del lazo (ref, error, u, ua, yk, ykd, running)
    
    // Componentes de control
    auto step = std::make_shared<SignalGenerator::StepSignal>(0.001, 1.0);
    auto sine = std::make_shared<SignalGenerator::SineSignal>(0.001, 1.0, 0.5);
    SignalGenerator::SignalSwitch sw(step, sine, 1);
    DiscreteSystems::PIDController pid(1.0, 0.5, 0.1, 0.001);
    DiscreteSystems::TransferFunctionSystem planta(/*...*/, 0.001);
    
    // Comunicación IPC
    Receptor receptor(&params);      // Objeto funcional
    Transmisor transmisor(&vars);    // Objeto funcional
    
    if (receptor.inicializar() && transmisor.inicializar()) {
        // Crear hilos especializados (cada uno ejecuta a frecuencia fija)
        HiloSwitch hiloSw(&sw, &vars.ref, &vars.running, &vars.mtx, &params, 100);
        HiloPID hiloPID(&pid, &vars, &params, 100);
        Hilo hiloSumador(&sumador, &vars.ref, &vars.error, &vars.running, &vars.mtx, 100);
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

La documentación completa y diagramas de arquitectura están en `doc/` y generados con Doxygen:
- `doc/ARCHITECTURE.md` (arquitectura y diagramas de flujo)
- `doc/mainpage.md` (resumen general y ejemplos)
- Documentación HTML generada por Doxygen en `doc/doxygen/html/index.html`


## 🔧 Configuración

La configuración centralizada (SSOT) está en `include/config/system_config.h` usando `constexpr`.

## 🐛 Troubleshooting


### Error: "could not open lock file"
Necesitas permisos de superusuario para instalar dependencias del sistema. Usa `sudo`.

### Error: "cannot create /queue"
Las colas POSIX requieren permisos. Si tienes problemas, asegúrate de que tu usuario pertenece al grupo adecuado o ejecuta con `sudo`. Verifica:
```bash
ls -la /dev/mqueue/
```
Si el directorio no existe, revisa la configuración de tu sistema (puede requerir activar POSIX message queues en el kernel).

### Tests fallan
Limpia y recompila:
```bash
rm -rf build/
# Recompilar desde cero
```


## 📖 Conceptos Clave

- **Instrumentación selectiva**: Solo los hilos de control están instrumentados con `RuntimeLogger` (buffer circular, flush periódico). Los hilos de comunicación IPC no generan logs para evitar overhead.
- **Modularización de señales**: Cada tipo de señal (`StepSignal`, `SineSignal`, `PwmSignal`, `SignalMixer`) tiene su propio header y fuente, facilitando la extensión y el mantenimiento.
- **Patrón NVI (Non-Virtual Interface)**: `DiscreteSystem::next()` es público y no-virtual; garantiza almacenamiento en buffer. Las subclases sobrescriben `compute()` protegido.
- **Buffer Circular**: Evita asignaciones dinámicas en el hot loop. Implementado con `std::deque` e índices manuales.
- **IPC con Serialización Manual**: Structs sin padding para portabilidad entre procesos. Uso de `serializeDataMessage()`.
- **Threading de Frecuencia Fija**: `Hilo`, `Hilo2in`, `HiloPID` usan `Temporizador` con `clock_nanosleep(TIMER_ABSTIME)` para mantener período constante sin drift acumulativo.


## 👥 Autoría

- **Autor**: Jordi
- **Asistencia**: GitHub Copilot
- **Proyecto**: Trabajo Final - Sistemas en Tiempo Real
- **Versión**: Enero 2026

## 📄 Licencia

Este proyecto es open source y material educativo. Consulta el archivo [LICENSE](LICENSE) para más detalles.

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
