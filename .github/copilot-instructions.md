# Instrucciones de Copilot para Proyecto PL7 Control de Sistemas Discretos

## Descripción General del Proyecto
Framework de control de sistemas en tiempo real desarrollado como Trabajo Final para la asignatura Sistemas en Tiempo Real. Implementado en C++17 con dos componentes principales:
1. **Librería Core** (`src/`, `include/`): Sistemas discretos C++17 reutilizables (PID, funciones de transferencia, generadores de señal)
2. **Interfaz Gráfica** (`Interfaz_Control/`): Interfaz gráfica Qt6 con visualización de PID en tiempo real y comunicación IPC

## Arquitectura y Flujos de Datos

### Sistemas Core
- **DiscreteSystem**: Clase base NVI (Non-Virtual Interface) para todos los sistemas discretos. El método público `next(double)` almacena muestras en buffer circular; las subclases solo sobrescriben el método protegido `compute()`
- **Generadores de Señal**: Composición con `std::shared_ptr<Signal>` (seno, escalón, rampa). Usa `compute()` para obtener valores sin avanzar el tiempo, `next()` para avanzar
- **Hilo** (threading): `Hilo` envuelve un `DiscreteSystem` con ejecución pthread a frecuencia fija (Hz). Protege variables compartidas con `std::mutex`
- **RuntimeLogger**: Instrumentación de diagnóstico solo en hilos de control (Hilo, Hilo2in, HiloPID, HiloSignal, HiloSwitch, HiloIntArranque). Hilos de comunicación IPC (HiloTransmisor, HiloReceptor) sin logging para reducir overhead

### Capa de GUI/Comunicación
- **Patrón IPC**: Colas de mensajes POSIX (mqueue) conectan proceso simulador → GUI Qt
  - Data Queue: Muestras en tiempo real (simulador envía, GUI recibe)
  - Params Queue: Actualizaciones de parámetros (GUI envía, simulador recibe)
- **Serialización Manual**: Sin padding de structs (seguro para endianness) mediante `serializeDataMessage()`/`deserializeParamsMessage()` en `Interfaz_Control/src/comm.cpp`
- **Tipos de Mensaje**: `DataMessage` (muestras) y `ParamsMessage` (Kp, Ki, Kd, setpoint)

### Estructura de Build
- **CMake Raíz** (`CMakeLists.txt`): Generación automática de tests desde archivos `test/*.cpp`
  - Patrón glob crea ejecutable para cada archivo test
  - Enlaza con librería estática `DiscreteSystems` + archivos `.a` en `lib/`
- **CMake Interfaz_Control** (`Interfaz_Control/CMakeLists.txt`): Build separado Qt6, copia headers a `include/` raíz
  - Salida en `Interfaz_Control/bin/` (gui_app, control_simulator, test_send, test_receive)

## Flujos de Trabajo de Desarrollo

### Compilación
```bash
# Build completo (core + GUI)
cd /home/jordi/PLs/PL7
./Interfaz_Control/build.sh

# Build manual (si el script falla)
cd build && cmake .. && make
cd Interfaz_Control && mkdir -p build && cd build && cmake .. && make
```

### Testing
- Tests unitarios en `test/*.cpp` auto-descubiertos por CMake
- Ejecutar test individual: `./bin/testPID`, `./bin/testTF`, etc.
- Muestras de salida guardadas en CSV/TSV en directorio `test/`

### Ejecutar Simulador GUI
```bash
./Interfaz_Control/bin/control_simulator &   # Simulador en background
./Interfaz_Control/bin/gui_app               # GUI Qt (conecta via IPC)
```

## Convenciones y Patrones Clave

### Espacios de Nombres
- `DiscreteSystems::`: Librería core (sistemas, señales, threading)
- Scope global: Interfaz_Control (IPC, GUI, simulador)

### Gestión de Memoria
- **Smart pointers**: Usa `std::shared_ptr<Signal>` en sistemas compuestos
- **Buffers circulares**: `std::deque<double>` en Signal; gestión manual de índices en DiscreteSystem
- **Sin malloc/free**: Solo contenedores STL

### Estándares de Código
- Estándar C++17 (definido en CMakeLists)
- Comentarios Doxygen para APIs públicas (ver `include/*.h`)
- Patrón NVI forzado: `next()` público no-virtual, `compute()` protegido virtual
- Indentación con tabulaciones en headers, 4 espacios en implementación

### Patrón de Testing
Crea nuevo test en `test/filename.cpp`:
```cpp
#include "../include/PIDController.h"
#include <iostream>

int main() {
    DiscreteSystems::PIDController pid(0.001, 10, 1.0, 0.5, 0.1);
    for(int i = 0; i < 100; i++) {
        double y = pid.next(1.0 - pid.compute());  // retroalimentación de error
    }
    // CMake auto-detecta y crea ejecutable "filename"
    return 0;
}
```

### Formato de Mensaje IPC
- Structs de tamaño fijo para seguridad en serialización (sin padding)
- Contador de secuencia en `DataMessage` para ordenamiento
- Recepción bloqueante en GUI; envíos no-bloqueantes en simulador

## Referencia de Archivos Críticos

| Archivo | Propósito |
|---------|-----------|
| `include/DiscreteSystem.h` | Clase base + patrón buffer circular |
| `include/SignalGenerator.h` | API de composición de señales reutilizable |
| `include/Hilo.h` | Wrapper de threading en tiempo real |
| `include/RuntimeLogger.h` | Instrumentación diagnóstico (buffer circular, flush periódico) |
| `include/system_config.h` | Configuración centralizada (SSOT): frecuencias, períodos, buffers |
| `Interfaz_Control/src/comm.h` | Interfaz de serialización IPC |
| `Interfaz_Control/src/messages.h` | Definiciones DataMessage/ParamsMessage |
| `CMakeLists.txt` (raíz) | Auto-descubrimiento de tests |
| `Interfaz_Control/CMakeLists.txt` | Build Qt6 + librería comm |

## Tareas Comunes

- **Agregar nuevo sistema discreto**: Crea `include/NewSystem.h`, implementa en `src/NewSystem.cpp`, hereda de `DiscreteSystem`
- **Agregar feature GUI**: Modifica `Interfaz_Control/src/mainwindow.cpp`; actualiza `messages.h` si se necesitan nuevos parámetros
- **Debug IPC**: Ejecuta `Interfaz_Control/bin/test_send` y `test_receive` independientemente para aislar comunicación
- **Profile de performance**: Buffer circular previene asignaciones en hot loop (`DiscreteSystem::next()`)

## Contexto Académico
Este proyecto es un Trabajo Final para Sistemas en Tiempo Real que demuestra:
- Separación de responsabilidades (math/control ≠ IPC ≠ GUI)
- Principios de tiempo real (threading de frecuencia fija, buffers circulares)
- Mejores prácticas C++17 (smart pointers, patrón NVI, RAII)

**Autor**: Jordi  
**Asistencia**: GitHub Copilot  
**Proyecto**: Trabajo Final - Sistemas en Tiempo Real
