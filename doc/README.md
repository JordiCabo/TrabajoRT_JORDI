# PL7 - Control de Sistemas Discretos

[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Documentation](https://img.shields.io/badge/docs-Doxygen-brightgreen.svg)](doc/doxygen/html/index.html)

## 📋 Descripción

Framework educativo de control de sistemas en tiempo real implementado en C++17. Proporciona una librería de sistemas discretos reutilizables (PID, funciones de transferencia, generadores de señal) y una interfaz gráfica Qt6 para visualización y control en tiempo real.

**Trabajo Final** para la asignatura de Sistemas en Tiempo Real.

### Características Principales

- 🎛️ **Controladores PID discretos** con sintonización en línea
- 📊 **Sistemas en espacio de estados** y funciones de transferencia
- 📡 **Generadores de señal** (escalón, rampa, senoidal, PWM)
- 🧵 **Ejecución multihilo** con frecuencia configurable
- 🖥️ **Interfaz gráfica Qt6** con visualización en tiempo real
- 🔄 **Comunicación IPC** mediante colas de mensajes POSIX
- 📈 **Visualización de gráficas** con Qt Charts
- 🔧 **Convertidores A/D y D/A** simulados

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
├── Interfaz_Control/     # Interfaz gráfica Qt6
│   ├── src/             # Código fuente GUI
│   ├── include/         # Headers IPC y comunicación
│   └── bin/             # Ejecutables compilados
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
- **Hilo/Hilo2in/HiloSignal**: Ejecución pthread a frecuencia fija

#### 2. Interfaz Gráfica (`Interfaz_Control/`)
- **GUI Qt6**: Ventana principal con gráficas en tiempo real
- **IPC**: Comunicación mediante POSIX message queues
- **Simulador**: Proceso separado que ejecuta el control PID
- **Serialización manual**: Sin padding para portabilidad

## 🚀 Compilación

### Requisitos

- **Compilador**: GCC/Clang con soporte C++17
- **CMake**: >= 3.10
- **Qt6**: Core, Gui, Charts
- **pthread**: Soporte POSIX threads
- **rt**: Extensiones de tiempo real (message queues)
- **Doxygen** (opcional): Para documentación
- **Graphviz** (opcional): Para diagramas

### Instalación de Dependencias

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake qt6-base-dev qt6-charts-dev \
                        libqt6charts6-dev doxygen graphviz
```

**Arch Linux:**
```bash
sudo pacman -S base-devel cmake qt6-base qt6-charts doxygen graphviz
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

# Compilar interfaz gráfica
cd Interfaz_Control
./build.sh
cd ..
```

### Build Manual por Partes

**Librería Core:**
```bash
cd build
cmake ..
make
```

**Interfaz Gráfica:**
```bash
cd Interfaz_Control
mkdir -p build && cd build
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

### Ejecutar Simulador con GUI

```bash
# Terminal 1: Iniciar simulador
./Interfaz_Control/bin/control_simulator &

# Terminal 2: Iniciar GUI
./Interfaz_Control/bin/gui_app
```

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

### Ejemplo: Sistema con Hilos

```cpp
#include "Hilo.h"
#include "PIDController.h"
#include <mutex>

int main() {
    std::mutex mtx;
    double ref = 1.0, feedback = 0.0, control = 0.0;
    bool running = true;
    
    DiscreteSystems::PIDController pid(1.0, 0.5, 0.1, 0.001);
    DiscreteSystems::Hilo hilo_pid(&pid, &ref, &control, &running, &mtx, 1000);
    
    // El hilo ejecuta automáticamente a 1000 Hz
    sleep(5);  // Simular 5 segundos
    
    running = false;  // Detener hilo
    return 0;
}
```

## 📚 Documentación

### Generar Documentación Doxygen

```bash
doxygen Doxyfile
xdg-open doc/doxygen/html/index.html
```

### Documentos Adicionales

- [Instrucciones Copilot](.github/copilot-instructions.md) - Guía para agentes IA
- [README Interfaz Control](Interfaz_Control/README.md) - Documentación GUI
- [Diseño GUI](Interfaz_Control/doc/DISEÑO_GUI.md)
- [Diseño Comunicación IPC](Interfaz_Control/doc/DISEÑO_COMUNICACION.md)

## 🔧 Configuración

### Parámetros de Compilación

Editar `CMakeLists.txt`:
```cmake
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3")
```

### Parámetros de Simulación

Editar `Interfaz_Control/src/config.h`:
```cpp
#define DEFAULT_FREQUENCY_HZ 1000
#define DEFAULT_KP 1.0
#define DEFAULT_KI 0.5
#define DEFAULT_KD 0.1
```

## 🐛 Troubleshooting

### Error: "Could not open lock file"
Necesitas permisos sudo para instalar dependencias.

### Error: Qt6 no encontrado
```bash
sudo apt-get install qt6-base-dev qt6-charts-dev
```

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
