# Evaluación del Proyecto PL7

Fecha de evaluación: 10/01/2026

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

## Debilidades
- Mutex único compartido: posible contención cuando el número de hilos aumenta.
- Falta de control de errores en `pthread_create/join`: sin verificación de códigos de retorno en algunos hilos.
- ~~Señales de control y variables compartidas en punteros crudos: riesgos de ciclo de vida si no se gestionan cuidadosamente.~~ **[RESUELTO]** Todas las clases `Hilo*` ahora usan smart pointers (`std::shared_ptr`) con interfaz dual para compatibilidad.
- Ausencia de registros (logging) para diagnósticos y trazabilidad.
- Sin CI/CD: no hay pipelines automáticos de build/test/análisis estático.
- Configuración de frecuencias/periodos dispersa: no existe una fuente única de verdad.
- Scheduling: no se configura `SCHED_FIFO/RR`; jitter depende de la carga del sistema.

## Recomendaciones (Corto Plazo)
- Mantener temporización absoluta actual (`Temporizador` + `TIMER_ABSTIME`) y documentar su uso en todos los hilos.
- Señal de parada: evaluar `std::atomic<bool> running` para reducir overhead y simplificar lectura.
- Errores de hilo: comprobar retornos de `pthread_create`/`pthread_join`; usar `pthread_exit(nullptr)`.
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
- Deriva temporal: mitigada por `Temporizador` con `TIMER_ABSTIME`; considerar `SCHED_FIFO/RR` si el jitter debe reducirse.
- Fugas/manejo de recursos: revisar retornos de API pthread; evitar asignaciones innecesarias en caminos de salida.

## Roadmap Sugerido
- Semana 1–2: corto plazo (atomic running, errores de hilos, logging, centralizar configuración).
- Semana 3–4: medio plazo (instrumentación, CI, mejoras de mutex, evaluación de scheduler).
- Mes 2+: largo plazo (buffers lock-free, perfiles, extensibilidad, portabilidad).

## Referencia de Implementación
- Ver `include/Hilo*.h` y `src/Hilo*.cpp` para patrón de acceso sincronizado (leer bajo mutex → computar fuera → escribir bajo mutex).
- Ver `include/Temporizador.h` y `src/Temporizador.cpp` para el patrón de temporización absoluta.
- Ver `doc/ARCHITECTURE.md` sección “Sincronización y Variables Compartidas” y “Ejecución en Tiempo Real” para detalles del modelo y temporización.
