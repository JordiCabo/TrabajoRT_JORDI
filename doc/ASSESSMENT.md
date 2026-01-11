# Evaluación del Proyecto PL7

Fecha de evaluación: 11/01/2026 (Actualización v1.0.6)

Este documento resume las fortalezas y debilidades del proyecto, e incluye recomendaciones prácticas de mejora a corto, medio y largo plazo.

## Fortalezas
- Arquitectura modular y clara: separación entre sistemas discretos, hilos, IPC y documentación.
- Patrón NVI en `DiscreteSystem`: interfaz estable con subclases especializadas.
- C++17 con RAII y smart pointers en los componentes principales.
- Documentación al día: Doxygen + docs en `doc/` actualizadas (README, ARCHITECTURE, mainpage).
- Tests auto-descubiertos por CMake en `test/` y binarios generados.
- Buffer circular y ejecución a frecuencia fija: enfoque realista para tiempo real blando.
- Temporización absoluta mediante `Temporizador` (`clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME)`), eliminando drift acumulativo.
- Utilidad `Discretizer` (método Tustin) para convertir plantas continuas a discretas según período de muestreo.
- Control de errores pthread robusto: verificación de retornos en todas las clases `Hilo*` (v1.0.5).
- Sincronización de variables compartidas con mutex protegiendo todas las operaciones críticas.
- **Sincronización robusta con timedlock (v1.0.6)**: 
  - RuntimeLogger con buffer circular de 1000 líneas para métricas sin impacto I/O excesivo.
  - Medición precisa de período real (Ts_Real_us) vs configurado: **error < 0.87%** en ejecución estable.
  - Análisis de jitter y drift: %error_Ts oscila entre ±~0.6%, con variación no acumulativa.
  - Tiempo de ejecución del PID: ~1-3 microsegundos con %uso < 0.03% del período (10 ms @ 100 Hz).
  - Timedlock con timeout del 20% para lectura de parámetros y escritura de salida, previniendo bloqueos indefinidos.
  - **Garantías de estabilidad**: márgenes muy amplios; sistema puede soportar cargas adicionales o reducir período sin comprometer estabilidad del regulador.

## Debilidades
- Mutex único compartido: posible contención cuando el número de hilos aumenta.
- Ausencia de registros (logging) para diagnósticos y trazabilidad.
- Sin CI/CD: no hay pipelines automáticos de build/test/análisis estático.
- Configuración de frecuencias/periodos dispersa: no existe una fuente única de verdad.
- Scheduling: no se configura `SCHED_FIFO/RR`; jitter depende de la carga del sistema.

## Recomendaciones (Corto Plazo)
- Logging básico: añadir un logger sencillo (timestamp, nombre de hilo, valores clave).
- Lecturas/escrituras: mantener cómputo (`next(...)`) fuera de la región crítica; consolidar lecturas en un único lock cuando sea posible.
- Centralizar configuración: definir `Config`/`comm_config.h` como fuente única de frecuencias y tamaños de buffer.

## Recomendaciones (Medio Plazo)
- Mutex por variable o estructura: considerar `SharedVars` con mutexes por flujo (ref/error/control/output).
- Planificador: evaluar `SCHED_FIFO`/`SCHED_RR` (Linux) con prioridades controladas para disminuir jitter.
- Instrumentación: medir jitter y latencias; exportar métricas (CSV/TSV) desde los hilos.
- Pruebas: añadir tests de integración del lazo con clocks simulados; pruebas de estrés.
- CI/CD: GitHub Actions (build + tests + `clang-tidy` + sanitizers).
- Configuración: centralizar periodos/frecuencias en `Config` (inmutable) pasado a hilos.

## Recomendaciones (Largo Plazo)
- Buffering: evaluar buffers lock-free (SPSC) para ciertos flujos si la carga aumenta.
- Propiedad de objetos: usar `std::unique_ptr` para sistemas envueltos por hilos; evitar punteros crudos.
- Trazas: soporte opcional de perfiles (trazas con marcas de tiempo) para analizar estabilidad temporal.
- Extensibilidad: plugin de sistemas (carga dinámica) con una interfaz estable.
- Portabilidad: abstracción de threading/temporización para entornos no-POSIX si se requiere.

## Riesgos y Mitigaciones
- Contención de mutex: mantener regiones críticas cortas; separar mutex por variable si crece el número de hilos.
- Deriva temporal: ✅ **MITIGADO** por `Temporizador` con `TIMER_ABSTIME`; considerar `SCHED_FIFO/RR` si el jitter debe reducirse aún más.
- Fugas/manejo de recursos: ✅ **MITIGADO en v1.0.5** revisando retornos de API pthread en todas las clases `Hilo*`; evitar asignaciones innecesarias en caminos de salida.

## Roadmap Sugerido
- Corto plazo (próximo): ✅ **COMPLETADO en v1.0.6**. Implementar timedlock con timeout del 20% en parámetros y salida de HiloPID. Extender RuntimeLogger a otros hilos (Hilo, Hilo2in, etc) con métodos writeLine() sobrecargados.
- Medio plazo: Instrumentación comparativa de jitter entre hilos, CI/CD con GitHub Actions, mutex por variable si contención lo requiere.
- Largo plazo: Buffers lock-free (SPSC), perfiles de trazas, extensibilidad con plugins, SCHED_FIFO/RR para reducir jitter del SO.

## Referencia de Implementación
- Ver `include/Hilo*.h` y `src/Hilo*.cpp` para patrón de acceso sincronizado (leer bajo mutex → computar fuera → escribir bajo mutex).
- Ver `include/Temporizador.h` y `src/Temporizador.cpp` para el patrón de temporización absoluta.
- Ver `doc/ARCHITECTURE.md` sección “Sincronización y Variables Compartidas” y “Ejecución en Tiempo Real” para detalles del modelo y temporización.
