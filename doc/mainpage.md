# PL7 - Control de Sistemas Discretos

## Trabajo Final - Sistemas en Tiempo Real

**Autor**: Jordi  
**Asistencia**: GitHub Copilot  
**Fecha**: Enero 2026

---

## Descripción General

Framework de control de sistemas en tiempo real implementado en C++17. Incluye librería core (PID, TF, SS, generadores de señal), utilidades de discretización (Discretizer con Tustin), temporización absoluta (Temporizador) y componentes IPC para sintonización en línea y visualización en GUI.

## Arquitectura de Lazo de Control Cerrado

El sistema implementa un lazo de control digital en tiempo real con la siguiente estructura:

```
                                                            ┌────────────────────────────────────────────────────────────────────────────┐
                                                            │                        LAZO DE CONTROL CERRADO                             │
                                                            └────────────────────────────────────────────────────────────────────────────┘

                                    ┌─────────────────┐            ┌───────────────────────────────────┐            ┌───────────────────────────────────┐
                                    │   Generador     │══════════► │  VARIABLES COMPARTIDAS            │            │  PARAMETROS COMPARTIDOS           │
                                    │   de Señales    │            │  (std::mutex protegidas)          │            │  (mutex POSIX protegidos)         │
                                    │  (Step/Sine)    │            │                                   │            │                                   │
                                    └────────┬────────┘            │ • ref(t)        Referencia        │            │ • Kp, Ki, Kd   Ganancias PID      │
                                             │ ref(t)              │ • e(t)          Error             │            │ • setpoint     Referencia         │
                                             │ (referencia)        │ • u(t)          Control PID       │            │ • signal_type  Tipo de señal      │
                                             ▼                     │ • u_analog(t)   Salida D/A        │            │                                   │
                                 ┌────────────────┐                │ • y(t)          Salida Planta     │            │                                   │
                           ┌────►│    Sumador     │<══════════════►│ • y_digital[k]  Retroalimentación │            │                                   │
                           │     │   (ref - y)    │                │                                   │            │                                   │
                           │     └────────┬───────┘                │                                   │            │                                   │
                           │              │ e(t)                   │                                   │            │                                   │
                           │              │ (error)                │                                   │            │                                   │
                           │              ▼                        │                                   │            │                                   │
                           │     ┌────────────────┐                │                                   │            │                                   │
                           │     │   Regulador    │<══════════════►│                                   │            │                                   │
                           │     │      PID       │                │                                   │            │                                   │
                           │     │  (Kp,Ki,Kd)    │                │                                   │            │                                   │
                           │     └────────┬───────┘                │                                   │            │                                   │
                           │              │ u(t)                   │                                   │            │                                   │
                           │              │ (control)              │                                   │            │                                   │
                           │              ▼                        │                                   │            │                                   │
                           │     ┌────────────────┐                │                                   │            │                                   │
                           │     │  Conversor D/A │<══════════════►│                                   │            │                                   │
                           │     │      (ZOH)     │                │                                   │            │                                   │
                           │     └────────┬───────┘                │                                   │            │                                   │
                           │              │ u_analog(t)            │                                   │            │                                   │
                           │              │                        │                                   │            │                                   │
                           │              ▼                        │                                   │            │                                   │
                           │     ┌────────────────┐                │                                   │            │                                   │
                           │     │     Planta     │<══════════════►│                                   │            │                                   │
                           │     │  G(s) o SS     │                │                                   │            │                                   │
                           │     │                │                │                                   │            │                                   │
                           │     └────────┬───────┘                │                                   │            │                                   │
                           │              │ y(t)                   │                                   │            │                                   │
                           │              │ (salida)               │                                   │            │                                   │
                           │              ▼                        │                                   │            │                                   │
                           │     ┌────────────────┐                │                                   │            │                                   │
                           │     │  Conversor A/D │<══════════════►│                                   │            │                                   │
                           │     │  (Muestreo)    │                │                                   │            │                                   │
                           │     └────────┬───────┘                │                                   │            │                                   │
                           │              │ y_digital[k]           │                                   │            │                                   │
                           └──────────────┘ (retroalimentación)    └───────────────────────────────────┘            └───────────────────────────────────┘
                                                                                      ▼                                             ▲
                                                                              ┌───────────────┐                             ┌───────────────┐
                                                                              │ TRANSMISOR    │                             │  RECEPTOR     │
                                                                              │ (DataMessage) │                             │(ParamsMessage)│
                                                                              └───────┬───────┘                             └───────┬───────┘
                                                                                      ▼                                             ▲
                                                                                      ▼                                             ▲
                                                                                      ▼                                             ▲
                                                                                      ┌─────────────────────────────────────────────┐
                                                                                      │                 GUI (Qt)                    │
                                                                                      └─────────────────────────────────────────────┘

                  Leyenda: ═══► Acceso lectura/escritura a variables compartidas protegidas por mutex
                      ◄───────► Comunicación IPC (DataMessage/ParamsMessage) entre simulador y GUI
                  ```

Variables Compartidas (protegidas por std::mutex):
   • ref         : Referencia del generador de señales
   • error       : Error = ref - y (salida del sumador)
   • u           : Acción de control del PID
   • ua          : Salida del conversor D/A
   • yk          : Salida de la planta
   • ykd         : Salida del conversor A/D (retroalimentación)

### Componentes IPC y sintonización en línea

- **ParametrosCompartidos**: Kp, Ki, Kd, setpoint y selector de señal (`signal_type`) protegidos con mutex POSIX.
- **VariablesCompartidas**: ref, error, u, ua, yk, ykd, running con mutex POSIX para el lazo principal.
- **Receptor/Transmisor**: Objetos funcionales que gestionan la recepción/envío de mensajes IPC (ParamsMessage/DataMessage).
- **HiloReceptor/HiloTransmisor**: Hilos periódicos que ejecutan los métodos de Receptor/Transmisor a frecuencia fija.
- **HiloPID**: Ejecuta PID leyendo parámetros dinámicamente en cada ciclo (sintonización en línea).
- **SignalSwitch/HiloSwitch**: Multiplexa step/rampa/seno/PWM leyendo `signal_type` actualizado por la GUI.


### Flujo de Datos

1. **Generador de Señales** (`SignalGenerator::Signal`)
   - Genera señal de referencia `ref(t)`
   - Tipos: Step, Sine, Ramp, PWM
   - Variable compartida: `reference_`

2. **Sumador** (`DiscreteSystems::Sumador`)
   - Calcula error: `e(t) = ref(t) - y_digital[k]`
   - Entrada: referencia y retroalimentación
   - Variable compartida: `error_`

3. **Regulador PID** (`DiscreteSystems::PIDController`)
   - Calcula acción de control: `u(t) = f(e(t), e(t-1), e(t-2))`
   - Parámetros: Kp, Ki, Kd ajustables en línea
   - Variable compartida: `control_`

4. **Conversor D/A** (`DiscreteSystems::DAConverter`)
   - Retenedor de orden cero (ZOH)
   - Mantiene `u(t)` constante durante Ts
   - Variable compartida: `control_analog_`

5. **Planta** (`TransferFunctionSystem` o `StateSpaceSystem`)
   - Sistema dinámico a controlar
   - G(s) = función de transferencia
   - Variable compartida: `plant_output_`

6. **Conversor A/D** (`DiscreteSystems::ADConverter`)
   - Muestrea salida de planta
   - Introduce retardo de 1 período (T_s)
   - Variable compartida: `feedback_`

7. **Transmisor (Emisor)** (`Transmisor`/`HiloTransmisor`)
   - Envía datos del sistema (ref, u, yk, timestamp) a la GUI mediante mensajes IPC (`DataMessage`)
   - Conexión: `/data_queue` → GUI Qt

8. **Receptor** (`Receptor`/`HiloReceptor`)
   - Recibe parámetros de la GUI (Kp, Ki, Kd, setpoint, tipo de señal) mediante mensajes IPC (`ParamsMessage`)
   - Conexión: `/params_queue` ← GUI Qt

### Ejecución en Tiempo Real

Cada bloque se ejecuta en un hilo pthread independiente (`Hilo`, `Hilo2in`, `HiloSignal`) a frecuencia fija configurable (típicamente 1000 Hz).

```cpp

// Ejemplo de configuración del lazo (versión real, comentarios didácticos)
std::mutex mtx; // Mutex global para sincronización
auto vars = std::make_shared<VariablesCompartidas>(); // Variables compartidas del lazo
auto params = std::make_shared<ParametrosCompartidos>(); // Parámetros PID y señal
auto running = std::make_shared<bool>(true); // Flag de ejecución

// Bloques del sistema (cada uno implementa la interfaz adecuada)
auto generator = std::make_shared<SignalGenerator::StepSignal>(Ts, amplitude); // Generador de referencia
auto sumador = std::make_shared<DiscreteSystems::Sumador>(Ts);                 // Calcula error
auto pid = std::make_shared<DiscreteSystems::PIDController>(Kp, Ki, Kd, Ts);   // Regulador PID
auto dac = std::make_shared<DiscreteSystems::DAConverter>(Ts);                 // Conversor D/A
auto planta = std::make_shared<DiscreteSystems::TransferFunctionSystem>(num, den, Ts); // Planta
auto adc = std::make_shared<DiscreteSystems::ADConverter>(Ts);                 // Conversor A/D

// Hilos de ejecución: cada uno ejecuta su bloque a frecuencia fija
SignalGenerator::HiloSignal hilo_gen(generator.get(), &vars->ref, running.get(), &mtx, freq_component, SystemConfig::HILO_REF_NAME); // Generador
DiscreteSystems::Hilo2in hilo_sumador(sumador.get(), &vars->ref, &vars->ykd, &vars->error, running.get(), &mtx, freq_component, SystemConfig::HILO_SUMADOR_NAME); // Sumador
DiscreteSystems::HiloPID hilo_regulador(pid.get(), vars.get(), params.get(), freq_controller, SystemConfig::HILO_PID_NAME); // Regulador PID
DiscreteSystems::Hilo hilo_dac(dac.get(), &vars->u, &vars->ua, running.get(), &mtx, freq_component, SystemConfig::HILO_DA_NAME); // D/A
DiscreteSystems::Hilo hilo_planta(planta.get(), &vars->ua, &vars->yk, running.get(), &mtx, freq_component, SystemConfig::HILO_PLANTA_NAME); // Planta
DiscreteSystems::Hilo hilo_adc(adc.get(), &vars->yk, &vars->ykd, running.get(), &mtx, freq_component, SystemConfig::HILO_AD_NAME); // A/D

// Hilos IPC opcionales (GUI en tiempo real)
HiloSwitch hilo_ref(&signalSwitch, &vars->ref, running.get(), &mtx, params.get(), 100);     // Referencia seleccionable
HiloPID hilo_pid_dyn(pid.get(), vars.get(), params.get(), 100, SystemConfig::HILO_PID_NAME); // PID con Kp/Ki/Kd dinámicos
HiloReceptor hilo_rx(&receptor, running.get(), &mtx, 50);                       // Recibe ParamsMessage
HiloTransmisor hilo_tx(&transmisor, running.get(), &mtx, 50);                   // Envía DataMessage
```

## Componentes Principales

### Namespace DiscreteSystems

- **DiscreteSystem**: Clase base abstracta con patrón NVI
- **PIDController**: Control PID discreto con ecuación en diferencias
- **TransferFunctionSystem**: Sistemas SISO con función de transferencia
- **StateSpaceSystem**: Representación en espacio de estados
- **ADConverter**: Muestreador A/D con retardo
- **DAConverter**: Retenedor de orden cero (ZOH)
- **Sumador**: Bloque restador para cálculo de error
- **Hilo/Hilo2in**: Wrappers pthread para ejecución en tiempo real (con `Temporizador`)
- **HiloPID**: Wrapper especializado de PID con lectura dinámica de Kp/Ki/Kd (con `Temporizador`)
- **HiloReceptor/HiloTransmisor**: Hilos periódicos para IPC (params/data) (con `Temporizador`)
- **HiloSwitch**: Hilo para multiplexado dinámico de referencia (con `Temporizador`)

### Utilidades de Discretización y Temporización

- **Discretizer**: Discretiza funciones de transferencia continuas B(s)/A(s) → B(z)/A(z) usando **método Tustin (bilineal)**. Permite convertir plantas analógicas a representación discreta con período de muestreo configurable.
- **Temporizador**: Proporciona temporización periódica **absoluta** mediante `clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME)`, eliminando drift acumulativo en loops de control. Usado por todos los hilos (`Hilo`, `HiloPID`, `HiloReceptor`, `HiloTransmisor`, `HiloSwitch`).

### Namespace SignalGenerator

- **Signal**: Clase base para generadores
- **SineSignal**: Señal senoidal
- **StepSignal**: Señal escalón
- **RampSignal**: Señal rampa
- **PWMSignal**: Modulación por ancho de pulso
- **HiloSignal**: Wrapper pthread para señales
- **SignalSwitch**: Multiplexor de señales (step/rampa/seno/PWM)



## Patrones de Diseño

- **NVI (Non-Virtual Interface)**: `DiscreteSystem::next()` garantiza almacenamiento y consistencia de buffer circular
- **RAII**: Gestión automática de recursos (threads, mutex, colas IPC)
- **Strategy**: Intercambio de generadores de señal y sistemas discretos
- **Dependency Injection**: Hilos reciben punteros a sistemas, facilitando testabilidad
- **Instrumentación selectiva**: Solo hilos de control instrumentados con `RuntimeLogger` (buffer circular, flush periódico)

## Características de Tiempo Real

- Ejecución pthread a frecuencia fija (Hz definida en `config/system_config.h`)
- Sincronización con `std::mutex`, `std::lock_guard` y `pthread_mutex_timedlock` (timeout 20% período)
- Temporización absoluta con `Temporizador` para eliminar drift (`clock_nanosleep` + `TIMER_ABSTIME`)
- **RuntimeLogger** con buffer circular para diagnóstico en tiempo real (solo hilos de control, logging selectivo)
- Signal handler (SIGINT/SIGTERM) para parada limpia sin errores pthread_join
- Error logging centralizado: stderr redirigido a `logs/error_log_YYYYMMDD_HHMMSS.txt`
- Configuración centralizada (SSOT) en `config/system_config.h`: frecuencias, períodos, buffers
- Buffer circular para evitar asignaciones dinámicas
- Variables compartidas protegidas en todo momento
- Código organizado en carpetas temáticas (`hilos/`, `sistemas/`, `senales/`, `converters/`, `io/`, `utilidades/`, `config/`)

## Uso Rápido


Ver ejemplos detallados en cada clase. Para comenzar:

```cpp
#include "sistemas/PIDController.h"

DiscreteSystems::PIDController pid(0.001, 10, 1.0, 0.5, 0.1);
for(int i = 0; i < 100; i++) {
   double y = pid.next(1.0 - pid.compute());  // retroalimentación de error
}
```

## Navegación

- Ver jerarquía de clases en el menú "Classes" (Doxygen)
- Buscar funciones específicas en "Class Members"
- Revisar archivos fuente en "Files" (organizados por carpeta temática)
- Consultar ejemplos y diagramas de herencia/colaboración en las páginas de cada clase

---

**Más información**: Consulta el README del proyecto (archivo README.md en la raíz).
