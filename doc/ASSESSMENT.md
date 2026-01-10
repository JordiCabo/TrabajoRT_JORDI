# Evaluación del Proyecto PL7

Fecha de evaluación: 10/01/2026 (v1.0.6)

Este documento resume las fortalezas y debilidades del proyecto, e incluye recomendaciones prácticas de mejora a corto, medio y largo plazo.

## Fortalezas
- Arquitectura modular y clara: separación entre sistemas discretos, hilos, IPC y documentación.
- Patrón NVI en `DiscreteSystem`: interfaz estable con subclases especializadas.
- C++17 con RAII y smart pointers: completamente migrado en v1.0.4 (shared_ptr en todas las clases Hilo*).
- Documentación al día: Doxygen + docs en `doc/` actualizadas (README, ARCHITECTURE, mainpage).
- Tests auto-descubiertos por CMake en `test/` y binarios generados.
- Buffer circular y ejecución a frecuencia fija: enfoque realista para tiempo real blando.
- Temporización absoluta mediante `Temporizador` (`clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME)`), eliminando drift acumulativo.
- Utilidad `Discretizer` (método Tustin) para convertir plantas continuas a discretas según período de muestreo.
- **v1.0.4: Ciclo de vida seguro mediante shared_ptr**: co-propiedad explícita de objetos compartidos entre hilos; eliminación automática de leaks.
- **v1.0.5: Control de errores robusto**: verificación de retornos en pthread_create/join con excepciones y logging.
- **v1.0.6: Instrumentación de timing en HiloPID**: trylock para evitar bloqueos, logging automático de métricas de rendimiento (t_espera, t_ejec, t_total, %uso), detección de contención de mutex y deadlines perdidos.

## Debilidades
- Mutex único compartido: posible contención cuando el número de hilos aumenta.
- Ausencia de registros (logging) para diagnósticos y trazabilidad.
- Sin CI/CD: no hay pipelines automáticos de build/test/análisis estático.
- Configuración de frecuencias/periodos dispersa: no existe una fuente única de verdad.
- Scheduling: no se configura `SCHED_FIFO/RR`; jitter depende de la carga del sistema.

## Recomendaciones (Corto Plazo)
- **v1.0.4 COMPLETADO**: Migración a shared_ptr en todas las clases Hilo* (Hilo, Hilo2in, HiloPID, HiloSignal, HiloSwitch, HiloIntArranque, HiloTransmisor, HiloReceptor). Co-propiedad explícita con acceso a POSIX mutex via `.get()`. Compilación exitosa, validación runtime completada.
- **v1.0.5 FASE 1 COMPLETADA**: Control de errores en threading implementado. Verificación de retornos de `pthread_create` con lanzamiento de `std::runtime_error` en caso de fallo. Verificación de retornos de `pthread_join` con advertencias en stderr. Todos los Hilo* (8 clases) actualizadas con headers `<iostream>` y `<stdexcept>`. Compilación 100% exitosa.
- **v1.0.6 COMPLETADA**: Instrumentación de timing en HiloPID con pthread_mutex_trylock() y logging automático. Detecta contención de mutex (umbral 80%), deadlines perdidos (>100% período), y registra métricas detalladas (t_espera, t_ejec, t_total, %uso) en `logs/HiloPID_runtime_*.txt`. Test dedicado validado con 200+ iteraciones.
- Mantener temporización absoluta actual (`Temporizador` + `TIMER_ABSTIME`) y documentar su uso en todos los hilos.
- Señal de parada: evaluar `std::atomic<bool> running` para reducir overhead y simplificar lectura.
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
- Propiedad de objetos: ✅ **COMPLETADO en v1.0.4** - shared_ptr implementado en todas las clases Hilo*; eliminación segura de punteros crudos.
- Trazas: soporte opcional de perfiles (trazas con marcas de tiempo) para analizar estabilidad temporal.
- Extensibilidad: plugin de sistemas (carga dinámica) con una interfaz estable.
- Portabilidad: abstracción de threading/temporización para entornos no-POSIX si se requiere.
- Reemplazar VariablesCompartidas.mtx (pthread_mutex_t) por `std::mutex` wrapper en v1.0.5 para mayor seguridad.

## Riesgos y Mitigaciones
- Contención de mutex: mantener regiones críticas cortas; separar mutex por variable si crece el número de hilos.
- Deriva temporal: mitigada por `Temporizador` con `TIMER_ABSTIME`; considerar `SCHED_FIFO/RR` si el jitter debe reducirse.
- Fugas/manejo de recursos: revisar retornos de API pthread; evitar asignaciones innecesarias en caminos de salida.

## Roadmap Sugerido
- **v1.0.4 (COMPLETADO)**: Migración a shared_ptr en threading (Fase 1-3); compilación exitosa; validación runtime.
- **v1.0.5 Fase 1 (COMPLETADO)**: Control de errores en `pthread_create/join` implementado en todas las 8 clases Hilo*. Excepciones lanzadas en constructores, advertencias en destructores.
- **v1.0.6 (COMPLETADO)**: Instrumentación de timing en HiloPID con trylock, logging automático de métricas de rendimiento, detección de contención y deadlines.
- **v1.0.5 Fase 2-5 (PENDIENTES)**: Logging general, configuración centralizada, mutex por variable, SCHED_FIFO/RR.
- Semana 2–3 (v1.0.7): corto plazo (logging general, centralizar configuración, mejoras de mutex, evaluación de scheduler).
- Semana 4–5 (v1.1.0): medio plazo (instrumentación, CI, evaluación avanzada).
- Mes 2+ (v2.0.0): largo plazo (buffers lock-free, perfiles, extensibilidad, portabilidad).

## Referencia de Implementación
- Ver `include/Hilo*.h` y `src/Hilo*.cpp` para patrón de acceso sincronizado (leer bajo mutex → computar fuera → escribir bajo mutex).
- Ver `include/Temporizador.h` y `src/Temporizador.cpp` para el patrón de temporización absoluta.
- Ver `doc/ARCHITECTURE.md` sección “Sincronización y Variables Compartidas” y “Ejecución en Tiempo Real” para detalles del modelo y temporización.
