# PL7 - Control de Sistemas Discretos

## Trabajo Final - Sistemas en Tiempo Real

**Autor**: Jordi  
**Asistencia**: GitHub Copilot  
**Fecha**: Enero 2026

---

## Descripción General

Framework de control de sistemas en tiempo real implementado en C++17. Incluye librería core (PID, TF, SS, generadores de señal) y componentes IPC para sintonización en línea y visualización en GUI.

## Arquitectura de Lazo de Control Cerrado

El sistema implementa un lazo de control digital en tiempo real con la siguiente estructura:

```
┌────────────────────────────────────────────────────────────────────────────┐
│                        LAZO DE CONTROL CERRADO                             │
└────────────────────────────────────────────────────────────────────────────┘

                              ┌─────────────────┐            ┌───────────────────────────────────┐
                              │   Generador     │══════════► │  VARIABLES COMPARTIDAS            │
                              │   de Señales    │            │  (std::mutex protegidas)          │
                              │  (Step/Sine)    │            │                                   │
                              └────────┬────────┘            │ • ref(t)        Referencia        │ 
                                       │ ref(t)              │ • e(t)          Error             │
                                       │ (referencia)        │ • u(t)          Control PID       │
                                       ▼                     │ • u_analog(t)   Salida D/A        │
                           ┌────────────────┐                │ • y(t)          Salida Planta     │
                     ┌────►│    Sumador     │<══════════════►│ • y_digital[k]  Retroalimentación │
                     │     │   (ref - y)    │                │                                   │
                     │     └────────┬───────┘                │                                   │
                     │              │ e(t)                   │                                   │
                     │              │ (error)                │                                   │
                     │              ▼                        │                                   │
                     │     ┌────────────────┐                │                                   │
                     │     │   Regulador    │<══════════════►│                                   │
                     │     │      PID       │                │                                   │
                     │     │  (Kp,Ki,Kd)    │                │                                   │
                     │     └────────┬───────┘                │                                   │
                     │              │ u(t)                   │                                   │
                     │              │ (control)              │                                   │
                     │              ▼                        │                                   │
                     │     ┌────────────────┐                │                                   │
                     │     │  Conversor D/A │<══════════════►│                                   │
                     │     │      (ZOH)     │                │                                   │
                     │     └────────┬───────┘                │                                   │
                     │              │ u_analog(t)            │                                   │
                     │              │                        │                                   │
                     │              ▼                        │                                   │
                     │     ┌────────────────┐                │                                   │
                     │     │     Planta     │<══════════════►│                                   │
                     │     │  G(s) o SS     │                │                                   │
                     │     │                │                │                                   │
                     │     └────────┬───────┘                │                                   │
                     │              │ y(t)                   │                                   │
                     │              │ (salida)               │                                   │
                     │              ▼                        │                                   │
                     │     ┌────────────────┐                │                                   │
                     │     │  Conversor A/D │<══════════════►│                                   │
                     │     │  (Muestreo)    │                │                                   │
                     │     └────────┬───────┘                │                                   │
                     │              │ y_digital[k]           │                                   │
                     └──────────────┘ (retroalimentación)    └───────────────────────────────────┘

Leyenda: ═══► Acceso lectura/escritura a variables compartidas protegidas por mutex
```

Variables Compartidas (protegidas por std::mutex):
   • ref(t)      : Referencia del generador de señales
   • e(t)        : Error = ref - y (salida del sumador)
   • u(t)        : Acción de control del PID
   • u_analog(t) : Salida del conversor D/A
   • y(t)        : Salida de la planta
   • y_digital[k]: Salida del conversor A/D (retroalimentación)

### Componentes IPC y sintonización en línea

- **ParametrosCompartidos**: Kp, Ki, Kd, setpoint y selector de señal (`signal_type`) protegidos con mutex POSIX.
- **VariablesCompartidas**: ref, e, u, ua, yk, ykd, running con mutex POSIX para el lazo principal.
- **Receptor/HiloReceptor**: Reciben `ParamsMessage` desde `/params_queue` y actualizan `ParametrosCompartidos` periódicamente.
- **Transmisor/HiloTransmisor**: Envían `DataMessage` a `/data_queue` con ref, u, yk y timestamp para la GUI.
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

### Ejecución en Tiempo Real

Cada bloque se ejecuta en un hilo pthread independiente (`Hilo`, `Hilo2in`, `HiloSignal`) a frecuencia fija configurable (típicamente 1000 Hz).

```cpp
// Ejemplo de configuración del lazo
std::mutex mtx;
bool running = true;

// Variables compartidas
double ref = 0.0, error = 0.0, control = 0.0;
double control_analog = 0.0, plant_output = 0.0, feedback = 0.0;

// Bloques del sistema
SignalGenerator::StepSignal generator(Ts, amplitude);
DiscreteSystems::Sumador sumador(Ts);
DiscreteSystems::PIDController pid(Kp, Ki, Kd, Ts);
DiscreteSystems::DAConverter dac(Ts);
DiscreteSystems::TransferFunctionSystem planta(num, den, Ts);
DiscreteSystems::ADConverter adc(Ts);

// Hilos de ejecución @ 1000 Hz
SignalGenerator::HiloSignal hilo_gen(&generator, &ref, &running, &mtx, 1000);
DiscreteSystems::Hilo2in hilo_sumador(&sumador, &ref, &feedback, &error, &running, &mtx, 1000);
DiscreteSystems::Hilo hilo_pid(&pid, &error, &control, &running, &mtx, 1000);
DiscreteSystems::Hilo hilo_dac(&dac, &control, &control_analog, &running, &mtx, 1000);
DiscreteSystems::Hilo hilo_planta(&planta, &control_analog, &plant_output, &running, &mtx, 1000);
DiscreteSystems::Hilo hilo_adc(&adc, &plant_output, &feedback, &running, &mtx, 1000);

// Hilos IPC opcionales (GUI en tiempo real)
HiloSwitch hilo_ref(&signalSwitch, &ref, &running, &mtx, &params, 100);     // Referencia seleccionable
HiloPID hilo_pid_dyn(&pid, &vars, &params, 100);                           // PID con Kp/Ki/Kd dinámicos
HiloReceptor hilo_rx(&receptor, &running, &mtx, 50);                       // Recibe ParamsMessage
HiloTransmisor hilo_tx(&transmisor, &running, &mtx, 50);                   // Envía DataMessage
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
- **Hilo/Hilo2in**: Wrappers pthread para ejecución en tiempo real
- **HiloPID**: Wrapper especializado de PID con lectura dinámica de Kp/Ki/Kd
- **HiloReceptor/HiloTransmisor**: Hilos periódicos para IPC (params/data)
- **HiloSwitch**: Hilo para multiplexado dinámico de referencia

### Namespace SignalGenerator

- **Signal**: Clase base para generadores
- **SineSignal**: Señal senoidal
- **StepSignal**: Señal escalón
- **RampSignal**: Señal rampa
- **PWMSignal**: Modulación por ancho de pulso
- **HiloSignal**: Wrapper pthread para señales
- **SignalSwitch**: Multiplexor de señales (step/rampa/seno/PWM)



## Patrones de Diseño

- **NVI (Non-Virtual Interface)**: `DiscreteSystem::next()` garantiza almacenamiento
- **RAII**: Gestión automática de recursos (threads, mutex)
- **Strategy**: Intercambio de generadores de señal
- **Dependency Injection**: Hilos reciben punteros a sistemas

## Características de Tiempo Real

- Ejecución pthread a frecuencia fija (Hz configurable)
- Sincronización con `std::mutex` y `std::lock_guard`
- Buffer circular para evitar asignaciones dinámicas
- Variables compartidas protegidas en todo momento

## Uso Rápido

Ver ejemplos detallados en cada clase. Para comenzar:

```cpp
#include "PIDController.h"
#include "TransferFunctionSystem.h"

// Ver clase PIDController para ejemplo completo
```

## Navegación

- Ver jerarquía de clases en el menú "Classes"
- Buscar funciones específicas en "Class Members"
- Revisar archivos fuente en "Files"
- Consultar ejemplos en las páginas de cada clase

---

**Más información**: Consulta el [README.md](README.md) del proyecto.
