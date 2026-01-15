
# Evaluación del Proyecto PL7

Fecha de evaluación: 15/01/2026 (Actualización v1.0.6, revisado)

Este documento resume las fortalezas y debilidades del proyecto, e incluye recomendaciones prácticas de mejora a corto, medio y largo plazo.

## Fortalezas
- API pública sencilla, robusta y documentada (Doxygen), fácil de integrar y mantener (ver testAPI.cpp).
- Arquitectura modular y clara: separación estricta entre sistemas discretos, hilos, IPC y documentación.
- Patrón NVI en `DiscreteSystem`: interfaz estable, subclases especializadas.
- C++17 con RAII y smart pointers en todos los componentes principales.
- Documentación al día: Doxygen + docs en `doc/` (README, ARCHITECTURE, mainpage).
- Tests auto-descubiertos por CMake en `test/` y binarios generados.
- Buffer circular y ejecución a frecuencia fija: enfoque realista para tiempo real blando.
- Temporización absoluta mediante `Temporizador` (`clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME)`), eliminando drift acumulativo.
- Utilidad `Discretizer` (Tustin) para convertir plantas continuas a discretas según período de muestreo.
- Control de errores pthread robusto: verificación de retornos en todas las clases `Hilo*` (v1.0.5+).
- Sincronización de variables compartidas con mutex protegiendo todas las operaciones críticas.
- Instrumentación avanzada: `RuntimeLogger` con buffer circular, flush periódico y logging selectivo (solo hilos de control, no IPC).
- Timedlock robusto: Timeout configurable (20% del período) en operaciones críticas de HiloPID.
- Nombres de hilos centralizados: definidos en configuración para trazabilidad y logging multi-instancia.
- Signal handler robusto: parada limpia de todos los hilos con SIGINT/SIGTERM, sin errores de pthread_join ni fugas de recursos.
- Modularización de señales: clases separadas para `Signal`, `StepSignal`, `SineSignal`, `PwmSignal`, `SignalMixer` (v1.0.9).
- Configuración centralizada (SSOT): `system_config.h` con constantes `constexpr` en `SystemConfig`.
- Medición precisa de período real (Ts_Real_us) vs configurado: **error < 0.87%** en ejecución estable.
- Análisis de jitter y drift: %error_Ts ±~0.6%, sin acumulación.
- Logging selectivo: solo hilos de control (Hilo, Hilo2in, HiloPID, HiloSwitch, HiloSignal, HiloIntArranque) generan logs; hilos de comunicación IPC (HiloTransmisor, HiloReceptor) sin overhead.
- Redirección automática de stderr a `error_log_YYYYMMDD_HHMMSS.txt` para captura centralizada de errores.
- Frecuencia de comunicación IPC optimizada a 10 Hz (100ms) para balance entre responsividad GUI y overhead del sistema.

### Fortalezas adicionales
- Facilidad de testing y mantenimiento: inyección de dependencias y auto-detección de tests por CMake facilitan ampliación y mantenimiento.
- Separación estricta de responsabilidades: lógica de control, IPC y GUI desacopladas, lo que facilita escalabilidad y depuración.
- Documentación exhaustiva: además de Doxygen, la carpeta `doc/` cubre arquitectura, instalación, troubleshooting y casos de uso.
- Ejemplos y scripts de prueba: scripts y binarios de test permiten validar cada componente de forma aislada.
- Cumplimiento de buenas prácticas C++17: uso consistente de RAII, smart pointers, STL y patrones de diseño modernos.

## Debilidades
- Sin CI/CD: no hay pipelines automáticos de build/test/análisis estático.
- Scheduling: no se configura `SCHED_FIFO/RR`; jitter depende de la carga del sistema operativo.
- Logs solo en archivos: no hay visualización en tiempo real de métricas (requiere análisis offline).
- No hay visualización en tiempo real de logs ni métricas; requiere análisis offline.
- No hay abstracción para portabilidad fuera de POSIX (threading, temporización).
- Extensibilidad limitada: no hay soporte para plugins o carga dinámica de nuevos sistemas.
- No hay visualización directa de logs/diagnóstico en la GUI (solo por archivos).

## Recomendaciones (Corto Plazo)
- Mantener cómputo (`next(...)`) fuera de la región crítica; consolidar lecturas en un único lock cuando sea posible.
- Visualización de logs: considerar herramienta de análisis en tiempo real o script para parseo de archivos `*_runtime_*.txt`.
- Añadir referencias cruzadas entre documentos clave (README, ARCHITECTURE, SIGNAL_HANDLING, CHANGELOG).

## Recomendaciones (Medio Plazo)
- Planificador: evaluar `SCHED_FIFO`/`SCHED_RR` (Linux) con prioridades controladas para disminuir jitter.
- Pruebas: añadir tests de integración del lazo con clocks simulados; pruebas de estrés.
- CI/CD: GitHub Actions (build + tests + `clang-tidy` + sanitizers).
- Análisis de logs: script Python/Bash para generar gráficas de jitter, drift y %uso desde archivos `*_runtime_*.txt`.
- Automatizar análisis de logs con scripts Python/Bash para gráficas de jitter, drift y %uso.
- Añadir soporte para visualización de métricas/logs en tiempo real en la GUI.

## Recomendaciones (Largo Plazo)
- Buffering: evaluar buffers lock-free (SPSC) para ciertos flujos si la carga aumenta.
- Propiedad de objetos: usar `std::unique_ptr` para sistemas envueltos por hilos; evitar punteros crudos.
- Trazas: soporte opcional de perfiles (trazas con marcas de tiempo) para analizar estabilidad temporal.
- Extensibilidad: plugin de sistemas (carga dinámica) con una interfaz estable.
- Portabilidad: abstracción de threading/temporización para entornos no-POSIX si se requiere.
- Añadir soporte multiplataforma y portabilidad a otros entornos (Windows, RTOS).

## Riesgos y Mitigaciones
- Deriva temporal: ✅ **MITIGADO** por `Temporizador` con `TIMER_ABSTIME`; considerar `SCHED_FIFO/RR` si el jitter debe reducirse aún más.
- Fugas/manejo de recursos: ✅ **MITIGADO en v1.0.5** revisando retornos de API pthread en todas las clases `Hilo*`; evitar asignaciones innecesarias en caminos de salida.
- Contención de mutex: ✅ **NO OBSERVADA en v1.0.6** - tiempos de espera < 2 μs, %uso < 0.03%, timedlock nunca disparó timeout. Mutex único compartido es suficiente para la carga actual.
- Parada abrupta del sistema: ✅ **MITIGADO en v1.0.6** con signal handler que captura SIGINT/SIGTERM y detiene hilos limpiamente, evitando errores pthread_join.

## Roadmap Sugerido
- Corto plazo: ✅ **COMPLETADO en v1.0.6**. 
  - ✅ Implementado timedlock con timeout del 20% en parámetros y salida de HiloPID.
  - ✅ RuntimeLogger extendido a todos los hilos de control con parámetros `log_prefix` y `frequency` obligatorios.
  - ✅ Error logging centralizado mediante redirección de stderr a archivo con timestamp.
  - ✅ Signal handler para parada limpia con Ctrl+C (SIGINT/SIGTERM).
  - ✅ Optimización de frecuencia IPC a 10 Hz (100ms) para reducir overhead.
  - ✅ Configuración centralizada en `system_config.h` (Single Source of Truth).
- Medio plazo: Instrumentación comparativa de jitter entre hilos, CI/CD con GitHub Actions, script de análisis de logs para gráficas de métricas, visualización de métricas en GUI.
- Largo plazo: Buffers lock-free (SPSC), perfiles de trazas, extensibilidad con plugins, SCHED_FIFO/RR para reducir jitter del SO, portabilidad multiplataforma.

- Para detalles de sincronización y threading: `include/Hilo*.h`, `src/Hilo*.cpp`.
- Temporización absoluta: `include/Temporizador.h`, `src/Temporizador.cpp`.
- Logging avanzado: `include/RuntimeLogger.h`, `src/RuntimeLogger.cpp`.
- Configuración centralizada: `include/system_config.h` (SSOT).
- Signal handler y shutdown: `test/testHilo.cpp` y `doc/SIGNAL_HANDLING.md`.
- Ejemplos de uso y diagramas: `doc/ARCHITECTURE.md`, `mainpage.md`.
- Cambios y mejoras: `CHANGELOG.md`.
