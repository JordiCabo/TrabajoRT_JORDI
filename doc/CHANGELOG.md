# Changelog

Todos los cambios notables en este proyecto serán documentados en este archivo.

El formato está basado en [Keep a Changelog](https://keepachangelog.com/es-ES/1.0.0/),
y este proyecto adhiere a [Semantic Versioning](https://semver.org/lang/es/).

## [1.0.6] - 2026-01-11

### Añadido
- **RuntimeLogger completo**: Sistema de logging con buffer circular de 1000 líneas para métricas de tiempo real:
  - Parámetros `log_prefix` y `frequency` obligatorios en todos los constructores de hilos.
  - Métodos `initializeHilo()` y `initializeHiloPID()` para configuración de columnas.
  - Flush automático cada 100 líneas sin impacto I/O excesivo.
  - Archivos log: `logs/{prefix}_runtime_YYYYMMDD_HHMMSS.txt`.
  - Métricas capturadas: iteration, t_espera_us, t_ejec_us, t_total_us, periodo_us, Ts_Real_us, drift_us, %error_Ts, %uso, status (OK/WARNING/CRITICAL).
- **Timedlock con timeout del 20%** en HiloPID para lectura de parámetros y escritura de salida:
  - Previene bloqueos indefinidos con `pthread_mutex_timedlock()`.
  - Log de errores ETIMEDOUT cuando se alcanza timeout.
- **Error logging centralizado**:
  - Redirección automática de stderr a `logs/error_log_YYYYMMDD_HHMMSS.txt`.
  - Captura thread-safe de todos los std::cerr del sistema.
- **Signal handler limpio**:
  - Captura SIGINT/SIGTERM para parada controlada de todos los hilos.
  - Evita errores pthread_join al detener threads limpiamente.
- **Configuración centralizada (SSOT)**:
  - Nuevo archivo `include/system_config.h` con namespace `SystemConfig`.
  - Constantes constexpr: TS_CONTROLLER, FREQ_COMPONENT, FREQ_COMMUNICATION, BUFFER_SIZE_LOGGER, etc.
  - Single Source of Truth para frecuencias, períodos, buffers y timeouts.

### Cambiado
- **Logs selectivos**: RuntimeLogger solo en hilos de control (Hilo, HiloPID, HiloSwitch, HiloSignal, HiloIntArranque); removido de hilos de comunicación (HiloTransmisor, HiloReceptor, Hilo2in) para reducir overhead.
- **Frecuencia IPC optimizada**: `freq_communication` reducida a 10 Hz (100ms) desde valores previos variables para balance entre responsividad GUI y overhead del sistema.
- **testHilo.cpp**: Actualizado para usar `SystemConfig::TS_CONTROLLER` y constantes centralizadas; añadido signal handler global.

### Técnico
- **Evidencia de estabilidad** (logs de producción):
  - Tiempos de espera mutex: < 2 μs (insignificante)
  - %uso del período: < 0.03% (margen 99.97%)
  - Error de período: < 0.87% (jitter ±0.6%)
  - 0 eventos WARNING/CRITICAL en ejecuciones completas
  - Timedlock timeout nunca disparado (configurado al 20%)
- **Contención de mutex**: No observada en logs; mutex único compartido es suficiente para la carga actual.

## [1.0.5] - 2026-01-11

### Añadido
- **Control de errores pthread**: Todas las clases `Hilo*` ahora verifican códigos de retorno:
  - `pthread_create`: Lanza `std::runtime_error` si falla, con mensaje en `stderr`.
  - `pthread_join`: Registra error en `stderr` si falla (no lanza excepción en destructor).
- **Includes necesarios**: `<iostream>` y `<stdexcept>` añadidos en clases que los necesitan.

### Cambiado
- **Variable running**: Revertida de `std::shared_ptr<std::atomic<bool>>` a `bool*` (puntero crudo) como en v1.0.3 para simplificar sincronización con mutex existente.
- **HiloPID**: Simplificado a interfaz única con punteros crudos (eliminada interfaz dual con smart pointers).
- **testHilo.cpp**: 
  - Usa `vars->ref`, `vars->e`, `vars->u`, `vars->yk` (no variables locales).
  - Inicialización: `running=false` controlado por `HiloIntArranque`, `vars->running=true` para `HiloPID`.
  - Protección mutex correcta para todas las variables compartidas.
  - Límite de 40 iteraciones y Ctrl+C funcional.
- **testHiloPID_circular.cpp, testHiloPID_timing.cpp**: Actualizados para usar `.get()` con nueva interfaz de HiloPID.

### Corregido
- **ASSESSMENT.md**: Debilidad "Falta de control de errores en pthread_create/join" marcada como **RESUELTA en v1.0.5**.
- **Flujo de datos**: HiloPID ahora procesa correctamente usando las mismas variables que otros hilos (`vars->e`, `vars->u`).

### Técnico
- Patrón de error handling: 
  ```cpp
  int ret = pthread_create(&thread_, nullptr, &Class::threadFunc, this);
  if (ret != 0) {
      std::cerr << "[Class] Error: pthread_create falló con código " << ret << std::endl;
      throw std::runtime_error("Class - pthread_create falló");
  }
  ```

## [1.0.4] - 2026-01-11

### Cambiado
- **Todas las clases Hilo\***: Refactorización completa de punteros crudos a `std::shared_ptr` (smart pointers) con interfaz dual para compatibilidad:
  - `Hilo`: `DiscreteSystem*` → `std::shared_ptr<DiscreteSystem>`, `double*` → `std::shared_ptr<double>`, `bool*` → `std::shared_ptr<std::atomic<bool>>`
  - `HiloSignal`: `Signal*` → `std::shared_ptr<Signal>`
  - `HiloPID`: `DiscreteSystem*`, `VariablesCompartidas*`, `ParametrosCompartidos*` → smart pointers
  - `HiloSwitch`: `SignalSwitch*` → `std::shared_ptr<SignalSwitch>`
  - `Hilo2in`: `DiscreteSystem*` → `std::shared_ptr<DiscreteSystem>`
  - `HiloReceptor`: `Receptor*` → `std::shared_ptr<Receptor>`
  - `HiloTransmisor`: `Transmisor*` → `std::shared_ptr<Transmisor>`
  - `HiloIntArranque`: `InterruptorArranque*` → `std::shared_ptr<InterruptorArranque>`
- Cada clase mantiene constructores deprecados con punteros crudos para compatibilidad hacia atrás.
- **Documentación (ASSESSMENT.md)**: Debilidad #20 ("Punteros crudos con riesgos de ciclo de vida") marcada como **RESUELTA**.

### Ventajas
- Gestión automática de memoria y ciclo de vida.
- Prevención de fugas de memoria y dangling pointers.
- Código más seguro y mantenible siguiendo mejores prácticas C++17.

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

## [Unreleased]

### Añadido
- (Pendiente)

### Cambiado
- (Pendiente)

### Corregido
- (Pendiente)

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

[Unreleased]: https://github.com/JordiCabo/TrabajoRT_JORDI/compare/v1.1.0...HEAD
[1.1.0]: https://github.com/JordiCabo/TrabajoRT_JORDI/compare/v1.0.0...v1.1.0
[1.0.0]: https://github.com/JordiCabo/TrabajoRT_JORDI/compare/v0.1.0...v1.0.0
[0.1.0]: https://github.com/JordiCabo/TrabajoRT_JORDI/releases/tag/v0.1.0
