# Changelog

Todos los cambios notables en este proyecto serán documentados en este archivo.

El formato está basado en [Keep a Changelog](https://keepachangelog.com/es-ES/1.0.0/),
y este proyecto adhiere a [Semantic Versioning](https://semver.org/lang/es/).

## [Unreleased]

### Añadido
- Documentación completa del proyecto (README, CONTRIBUTING, CHANGELOG)
- Comentarios Doxygen en todos los archivos `.h` y `.cpp`
- Configuración Doxygen para generación de documentación HTML
- Licencia MIT

## [1.0.0] - 2024-12-18

### Añadido
- Clase base `DiscreteSystem` con patrón NVI
- Controlador `PIDController` con ecuación en diferencias
- Sistema `TransferFunctionSystem` para funciones de transferencia
- Sistema `StateSpaceSystem` para representación en espacio de estados
- Convertidores `ADConverter` y `DAConverter` simulados
- Clase `Sumador` para cálculo de error (dos entradas)
- Generadores de señal en namespace `SignalGenerator`:
  - `Signal`: Clase base abstracta
  - `SineSignal`: Señal senoidal
  - `StepSignal`: Señal escalón
  - `RampSignal`: Señal rampa
  - `PWMSignal`: Modulación por ancho de pulso
- Wrappers de threading:
  - `Hilo`: Ejecución de sistema discreto con una entrada
  - `Hilo2in`: Ejecución de sistema con dos entradas
  - `HiloSignal`: Ejecución de generador de señal
- Sistema de build CMake con auto-descubrimiento de tests
- Interfaz gráfica Qt6 (`Interfaz_Control/`):
  - Ventana principal con gráficas en tiempo real
  - Control de parámetros PID (Kp, Ki, Kd, Setpoint)
  - Visualización con Qt Charts
- Comunicación IPC mediante POSIX message queues:
  - Librería `comm` con serialización manual
  - Tipos de mensaje: `DataMessage`, `ParamsMessage`
  - Cola de datos (simulador → GUI)
  - Cola de parámetros (GUI → simulador)
- Ejecutables:
  - `control_simulator`: Simulador de sistema con PID
  - `gui_app`: Aplicación gráfica Qt6
  - `test_send` / `test_receive`: Tests de comunicación IPC
- Tests unitarios en directorio `test/`:
  - `testPID`: Test de controlador PID
  - `testTF`: Test de función de transferencia
  - `testSS`: Test de espacio de estados
  - `testStepSignal`: Test de generador de escalón
  - `testADConverter` / `testDAConverter`: Tests de conversores
  - `testHilo`: Test de ejecución con hilos
  - `testSumador`: Test de sumador de dos entradas

### Documentación
- README principal con arquitectura y ejemplos
- Documentación Doxygen completa
- Instrucciones Copilot para agentes IA (`.github/copilot-instructions.md`)
- Documentación específica de interfaz gráfica (`Interfaz_Control/README.md`)
- Diseño de comunicación IPC (`Interfaz_Control/doc/DISEÑO_COMUNICACION.md`)
- Diseño de GUI (`Interfaz_Control/doc/DISEÑO_GUI.md`)
- Guía de desarrollo (`Interfaz_Control/doc/DEVELOPMENT.md`)

### Técnico
- Estándar C++17
- Buffer circular en `DiscreteSystem` para evitar asignaciones dinámicas
- Patrón NVI para garantizar almacenamiento correcto de muestras
- Smart pointers (`std::shared_ptr`) para gestión de memoria
- Mutex (`std::mutex`) para sincronización entre hilos
- Serialización manual de structs para portabilidad IPC
- Frecuencia de ejecución configurable en Hz

## [0.1.0] - 2024-11-01

### Añadido
- Estructura inicial del proyecto
- Configuración CMake básica
- Clases base preliminares

---

## Tipos de Cambios

- **Añadido** - para nuevas funcionalidades
- **Cambiado** - para cambios en funcionalidades existentes
- **Obsoleto** - para funcionalidades que se eliminarán
- **Eliminado** - para funcionalidades eliminadas
- **Corregido** - para corrección de bugs
- **Seguridad** - para vulnerabilidades

[Unreleased]: https://github.com/USUARIO/PL7/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/USUARIO/PL7/compare/v0.1.0...v1.0.0
[0.1.0]: https://github.com/USUARIO/PL7/releases/tag/v0.1.0
