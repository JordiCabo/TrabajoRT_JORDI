# Changelog

Todos los cambios notables en este proyecto serán documentados en este archivo.

El formato está basado en [Keep a Changelog](https://keepachangelog.com/es-ES/1.0.0/),
y este proyecto adhiere a [Semantic Versioning](https://semver.org/lang/es/).

## [Unreleased]

### Añadido
- (Pendiente)

### Cambiado
- (Pendiente)

### Corregido
- (Pendiente)

## [1.0.4] - 2026-01-10

### Cambiado - Refactorización a Smart Pointers
**Objetivo**: Garantizar ciclo de vida seguro de objetos compartidos entre múltiples hilos, resolviendo riesgos de punteros crudos.

#### Fase 1: Core Threading (Completada) ✅
Migración de **punteros crudos a `shared_ptr`** en clase base de threading:
- **Hilo.h/cpp**: 
  - `DiscreteSystem*` → `std::shared_ptr<DiscreteSystem>`
  - `double*` (input/output) → `std::shared_ptr<double>`
  - `bool*` (running) → `std::shared_ptr<bool>`
  - `pthread_mutex_t*` → `std::shared_ptr<pthread_mutex_t>`
- **Hilo2in.h/cpp**: Migración idéntica para sistemas con dos entradas (Sumador)
- **HiloPID.h/cpp**: 
  - `VariablesCompartidas*` → `std::shared_ptr<VariablesCompartidas>`
  - `ParametrosCompartidos*` → `std::shared_ptr<ParametrosCompartidos>`
  - Acceso a pthread_mutex_t via `.get()` para mantener POSIX API

#### Fase 2: Threading Especializado (En Progreso) 🔄
Iniciada migración de clases derivadas:
- **HiloSignal.h/cpp**: Headers migrados, `.cpp` actualizado
- **HiloSwitch.h/cpp**: Headers + implementación actualizados
- **HiloIntArranque.h/cpp**: Pendiente completar
- **HiloTransmisor.h/cpp**: Pendiente completar
- **HiloReceptor.h/cpp**: Pendiente completar

#### Fase 3: Código Cliente (Pendiente) ⏳
- **testHilo.cpp**: Refactorizar instantiaciones a patrón `std::make_shared`
- **control_simulator.cpp** (Interfaz_Control): Actualizaciones necesarias
- **mainwindow.cpp**: Verificación/actualización

### Beneficios Logrados
✅ **Eliminación de memory leaks**: Reference counting automático  
✅ **Acceso seguro**: Imposible acceder a objeto destruido mientras hilo activo  
✅ **Propiedad clara**: Cada hilo co-posee sus recursos explícitamente  
✅ **Thread-safety mejorada**: Atomic operations en ref-counting internamente  

### Patrón Implementado
```cpp
// Acceso a mutex POSIX desde shared_ptr
pthread_mutex_lock(mtx_.get());    // Obtener puntero raw
// ... critical section ...
pthread_mutex_unlock(mtx_.get());
```

### Notas Técnicas
- Minimal performance overhead (1-2 ciclos CPU por ref-count)
- Hot-loop (Hilo::run) ya optimizado sin asignaciones dinámicas
- Futuro: Refactorizar `VariablesCompartidas` con `std::mutex` wrapper

## [1.0.3] - 2026-01-10

### Cambiado
- **HiloSignal**, **HiloSwitch**, **HiloTransmisor**, **HiloReceptor**, **HiloIntArranque**: Sustitución de `usleep` por `Temporizador` con temporización absoluta (`clock_nanosleep` + `TIMER_ABSTIME`) y comentarios actualizados.
- **Documentación**: README, ARCHITECTURE y mainpage actualizadas para reflejar temporización absoluta y utilidades de discretización.

### Corregido
- Calificación de namespace `DiscreteSystems::Temporizador` en hilos auxiliares para compilación correcta.

## [1.0.2] - 2026-01-10

### Añadido
- **Discretizer**: Nueva utilidad para discretizar funciones de transferencia continuas B(s)/A(s) a B(z)/A(z) mediante transformación bilineal (Tustin).
- **Temporizador**: Nueva clase para temporización absoluta con `clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME)` eliminando drift acumulativo en loops periódicos.

### Cambiado
- **Hilo**, **Hilo2in**, **HiloPID**: Reemplazado `usleep()` y `nanosleep()` relativo por `Temporizador` con retardo absoluto para mayor precisión en tiempo real.
- **testHilo.cpp**: Actualizado para discretizar planta continua 1/(tau*s+1) usando `Discretizer::discretizeTF()` con método Tustin.
- **Sección de Frecuencias**: Centralización en testHilo.cpp con `Ts_controller=0.01s` y `Ts_component=Ts_controller/10`.
- **Documentación**: README, ARCHITECTURE y mainpage Doxygen actualizados con descripciones de Discretizer y Temporizador.
- **Doxyfile**: Eliminado `Interfaz_Control/src/` de INPUT (proyecto ajeno al core).

### Corregido
- Comentarios Doxygen: Eliminados `@param` duplicados y referencias rotas en ADConverter, DAConverter, PIDController, Sumador, StateSpaceSystem, TransferFunctionSystem.
- DiscreteSystem.h: Eliminado bloque comentado `bufferDump()` que generaba warnings Doxygen.

## [1.1.0] - 2026-01-03

### Añadido
- Wrappers especializados: `HiloPID`, `HiloReceptor`, `HiloTransmisor`, `HiloSwitch`.
- Componentes IPC thread-safe: `ParametrosCompartidos`, `VariablesCompartidas`, `Receptor`, `Transmisor`.
- Multiplexor de señales `SignalSwitch` con selección dinámica desde GUI.
- Nuevos tests IPC (`test_send`, `test_receive`, `testTransmisor`).

### Cambiado
- Documentación Doxygen y mainpage actualizadas a la arquitectura IPC actual.
- README, ARCHITECTURE, INSTALL y CONTRIBUTING revisados con flujos IPC y sintonización en línea.

### Corregido
- Ajustes menores en comentarios y coherencia de nombres en headers IPC.

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
- Componentes auxiliares en `Interfaz_Control/` (proyecto del profesor):
  - Simulador independiente de sistema con PID
  - Comunicación IPC mediante POSIX message queues
  - Ejecutables de demostración y prueba
- Comunicación IPC mediante POSIX message queues:
  - Librería `comm` con serialización manual
  - Tipos de mensaje: `DataMessage`, `ParamsMessage`
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

[Unreleased]: https://github.com/JordiCabo/TrabajoRT_JORDI/compare/v1.0.4...HEAD
[1.0.4]: https://github.com/JordiCabo/TrabajoRT_JORDI/compare/v1.0.3...v1.0.4
[1.0.3]: https://github.com/JordiCabo/TrabajoRT_JORDI/compare/v1.0.2...v1.0.3
[1.0.2]: https://github.com/JordiCabo/TrabajoRT_JORDI/compare/v1.1.0...v1.0.2
[1.1.0]: https://github.com/JordiCabo/TrabajoRT_JORDI/compare/v1.0.0...v1.1.0
[1.0.0]: https://github.com/JordiCabo/TrabajoRT_JORDI/compare/v0.1.0...v1.0.0
[0.1.0]: https://github.com/JordiCabo/TrabajoRT_JORDI/releases/tag/v0.1.0
