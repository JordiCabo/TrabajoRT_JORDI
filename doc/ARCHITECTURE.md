# Arquitectura del Proyecto PL7

Este documento describe la arquitectura de alto nivel del sistema de Control de Sistemas Discretos.

## 📐 Diagrama de Arquitectura

```
┌─────────────────────────────────────────────────────────────────┐
│                   LIBRERÍA CORE DISCRETESYSTEMS                 │
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │         Sistemas Discretos (C++17 STL-only)              │  │
│  │                                                          │  │
│  │  - DiscreteSystem (base NVI)                            │  │
│  │  - PIDController, TransferFunctionSystem, etc.          │  │
│  │  - SignalGenerator (Step, Sine, Ramp, PWM)             │  │
│  │  - Hilo/Hilo2in/HiloSignal (threading)                 │  │
│  │  - ADConverter/DAConverter/Sumador                      │  │
│  │                                                          │  │
│  │  Buffer circular | Patrón NVI | Tests unitarios        │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│            COMPONENTES AUXILIARES (Interfaz_Control)            │
│                                                                 │
│  Simulador + IPC (Proyecto de demostración)                    │
│                                                                 │
│  - control_simulator: Ejecuta lazo de control                 │
│  - comm: POSIX message queues para comunicación               │
│  - gui_app: Interfaz visual (proyecto del profesor)           │
│  - Serialización manual: Sin padding                          │
└─────────────────────────────────────────────────────────────────┘
```

## 🏗️ Capas del Sistema

### 1. Capa de Dominio (Core Library)

**Ubicación**: `src/`, `include/`  
**Namespace**: `DiscreteSystems`, `SignalGenerator`

Implementa la lógica de control y sistemas discretos:

```cpp
DiscreteSystem (abstracta)
    │
    ├── PIDController
    ├── TransferFunctionSystem
    ├── StateSpaceSystem
    ├── ADConverter
    ├── DAConverter
    └── Sumador

Signal (abstracta)
    │
    ├── SineSignal
    ├── StepSignal
    ├── RampSignal
    └── PWMSignal
```

**Responsabilidades**:
- Implementar algoritmos de control discreto
- Gestionar buffers circulares de muestras
- Proporcionar API reutilizable y testeable

### 2. Capa de Threading

**Ubicación**: `include/Hilo*.h`, `src/Hilo*.cpp`

Wrappers para ejecución en tiempo real:

```cpp
Hilo          // 1 entrada  → 1 salida
Hilo2in       // 2 entradas → 1 salida
HiloSignal    // Generador de señal → 1 salida
```

**Responsabilidades**:
- Ejecutar sistemas a frecuencia fija (Hz)
- Sincronizar acceso con `std::mutex`
- Gestión de lifecycle de threads (`pthread`)

## 🔒 Sincronización y Variables Compartidas

Esta sección detalla cómo se sincronizan los hilos y cómo se realiza el acceso a las variables compartidas del lazo de control.

### Modelo de Concurrencia

- Se utiliza un `std::mutex` compartido entre todos los hilos (`Hilo`, `Hilo2in`, `HiloSignal`).
- Los accesos a variables compartidas (`ref`, `error`, `u`, `u_analog`, `y`, `y_digital`, y `running`) se realizan exclusivamente dentro de regiones críticas protegidas mediante `std::lock_guard<std::mutex>`.
- La computación del sistema (`system_->next(...)`) se ejecuta fuera de la sección crítica para minimizar el tiempo de bloqueo y evitar contención.
- Cada hilo impone su período de muestreo mediante `usleep(period_us)`, donde `period_us = 1e6 / frequency_`.

### Patrón de Acceso (canónico)

```cpp
// Ejecución a frecuencia fija con acceso sincronizado
int sleep_us = static_cast<int>(1e6 / frequency_);
while (true) {
        bool isRunning;
        { std::lock_guard<std::mutex> lock(*mtx_); isRunning = *running_; }
        if (!isRunning) break;

        // Leer entradas bajo mutex (copiar a variables locales)
        double in1, in2;
        { std::lock_guard<std::mutex> lock(*mtx_); in1 = *input1_; in2 = *input2_; }

        // Calcular salida fuera del lock
        double y = system_->next(in1, in2);

        // Escribir salida bajo mutex
        { std::lock_guard<std::mutex> lock(*mtx_); *output_ = y; }

        usleep(sleep_us);
}
```

Este patrón se aplica análogamente en `Hilo` (1 entrada → 1 salida) y `HiloSignal` (generador → 1 salida).

### Mapa de Lectura/Escritura por Hilo

- `HiloSignal` (generador de referencia):
    - Escribe: `ref`
    - Lee: `running`

- `Hilo2in` (sumador):
    - Lee: `ref`, `y_digital`, `running`
    - Escribe: `error`

- `Hilo` (PID controlador):
    - Lee: `error`, `running`
    - Escribe: `u`

- `Hilo` (Conversor D/A - ZOH):
    - Lee: `u`, `running`
    - Escribe: `u_analog`

- `Hilo` (Planta - TF/SS):
    - Lee: `u_analog`, `running`
    - Escribe: `y`

- `Hilo` (Conversor A/D):
    - Lee: `y`, `running`
    - Escribe: `y_digital`

### Principios de Diseño

- **Regiones críticas cortas**: leer/escribir bajo mutex y computar fuera.
- **Sin deadlocks**: un único mutex compartido, sin bloqueos anidados.
- **Jitter controlado**: el período se mantiene con `usleep`, el tiempo bajo lock es mínimo.
- **Terminación ordenada**: cada hilo verifica `running` bajo mutex y finaliza limpiamente.

### Observaciones Operativas

- Si se requiere mayor paralelismo, puede considerarse un mutex por variable; el diseño actual prioriza simplicidad y seguridad.
- La frecuencia de los hilos debe ser coherente con el período de muestreo del lazo para evitar aliasing o desincronización.

## 🔀 Diagrama de Flujo: Hilos ↔ Bloques

Relación entre las clases de hilos y los bloques que envuelven:

```
┌──────────────────────────────┐      wraps     ┌──────────────────────────────┐
│            Hilo              │ ──────────────►│        DiscreteSystem        │
│    (1 entrada → 1 salida)    │                │  PID, TF, SS, DA, AD, ...    │
└──────────────────────────────┘                └──────────────────────────────┘

┌──────────────────────────────┐      wraps     ┌──────────────────────────────┐
│           Hilo2in            │ ──────────────►│   DiscreteSystem (Sumador)   │
│    (2 entradas → 1 salida)   │                │      next(in1, in2)          │
└──────────────────────────────┘                └──────────────────────────────┘

┌──────────────────────────────┐      wraps     ┌──────────────────────────────┐
│          HiloSignal          │ ──────────────►│   SignalGenerator::Signal    │
│      (signal → 1 salida)     │                │    Step / Sine / Ramp / PWM  │
└──────────────────────────────┘                └──────────────────────────────┘

Notas:
- "wraps" indica que el hilo recibe un puntero al bloque y ejecuta su `next(...)` a frecuencia fija.
- La lectura/escritura de variables compartidas se gestiona con `std::mutex` (ver sección de Sincronización).
```

### Correspondencia típica en el lazo

- `HiloSignal` → envuelve `Signal` ⇒ escribe `ref`
- `Hilo2in`    → envuelve `Sumador` ⇒ lee `ref`, `y_digital` y escribe `error`
- `Hilo`       → envuelve `PIDController` ⇒ lee `error` y escribe `u`
- `Hilo`       → envuelve `DAConverter` ⇒ lee `u` y escribe `u_analog`
- `Hilo`       → envuelve `TransferFunctionSystem`/`StateSpaceSystem` ⇒ lee `u_analog` y escribe `y`
- `Hilo`       → envuelve `ADConverter` ⇒ lee `y` y escribe `y_digital`

### 3. Capa de Comunicación Inter-Procesos (IPC)

**Ubicación**: `include/`, `src/` (clases IPC principales)  
**Namespace**: Global (Receptor, Transmisor, ParametrosCompartidos, VariablesCompartidas)

Sistema de comunicación entre procesos para visualización en tiempo real y sintonización dinámica:

#### 3.1 Estructuras Compartidas

```cpp
class ParametrosCompartidos {
    double kp, ki, kd;          // Ganancias PID (sintonizables)
    double setpoint;            // Referencia deseada
    int signal_type;            // Tipo de señal (1=step, 2=ramp, 3=sine)
    pthread_mutex_t mtx;        // Protección thread-safe
};

class VariablesCompartidas {
    double ref;                 // Referencia del sistema
    double e;                   // Error: e(k) = ref - ykd
    double u;                   // Acción de control PID
    double ua;                  // Control analógico (post D/A)
    double yk;                  // Salida de planta (analógica)
    double ykd;                 // Salida digitalizada (post A/D)
    bool running;               // Flag de ejecución
    pthread_mutex_t mtx;        // Protección thread-safe
};
```

#### 3.2 Componentes de Comunicación

```cpp
class Receptor {
    // Recibe ParamsMessage desde mqueue
    // Actualiza ParametrosCompartidos con lock
    bool recibir();
};

class Transmisor {
    // Lee VariablesCompartidas con lock
    // Envía DataMessage a mqueue
    bool enviar();
};
```

#### 3.3 Hilos Especializados para IPC

```cpp
class HiloReceptor {
    // Ejecuta Receptor::recibir() periódicamente
    // Permite cambios de parámetros sin interrumpir el lazo
};

class HiloTransmisor {
    // Ejecuta Transmisor::enviar() periódicamente
    // Envía muestras a GUI a frecuencia controlada (típicamente 50 Hz)
};

class HiloPID {
    // Especialización de Hilo para PIDController
    // Lee parámetros dinámicamente de ParametrosCompartidos cada ciclo
    // Permite sintonización en línea sin recrear el controlador
};

class HiloSwitch {
    // Ejecuta SignalSwitch periódicamente
    // Lee signal_type de ParametrosCompartidos para cambiar generador
    // Permite cambiar entre escalón/rampa/senoidal en tiempo real
};
```

#### 3.4 Arquitectura del Sistema Completo

```
┌─────────────────────────────────────────────────────────────────┐
│               Proceso Simulador (control_simulator)              │
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │          Variables Compartidas (mutex-protegidas)        │   │
│  │  VariablesCompartidas: ref, e, u, ua, yk, ykd, running │   │
│  └────────────────┬────────────────────────────────────────┘   │
│                   │                                             │
│                   │  (lectura/escritura bajo mutex)             │
│                   │                                             │
│  ┌────────────────v──────┐  ┌──────────────────┐              │
│  │  HiloSignal (100Hz)   │  │  HiloPID(100Hz)  │              │
│  │  genera ref           │  │  controla planta │              │
│  │  lee: signal_type     │  │  lee: e,kp,ki,kd│              │
│  │  escribe: ref         │  │  escribe: u      │              │
│  └───────────────────────┘  └──────────────────┘              │
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │    HiloReceptor (50Hz)         HiloTransmisor (50Hz)     │  │
│  │                                                           │  │
│  │  Lee desde IPC:          Escribe hacia IPC:              │  │
│  │  ParamsMessage           DataMessage                     │  │
│  │  (Kp, Ki, Kd)           (ref, u, yk, tiempo)            │  │
│  │                                                           │  │
│  │  Actualiza:              Lee:                            │  │
│  │  ParametrosCompartidos   VariablesCompartidas            │  │
│  └────────┬───────────────────────────────┬─────────────────┘  │
│           │                               │                    │
│           └──────────────┬────────────────┘                    │
│                          │                                     │
└──────────────────────────┼─────────────────────────────────────┘
                           │
                POSIX Message Queues
                           │
        ┌──────────────────┴──────────────────┐
        │                                     │
        v                                     v
┌──────────────────┐              ┌──────────────────┐
│   /data_queue    │              │ /params_queue    │
│  (DataMessage)   │              │(ParamsMessage)   │
└──────────┬───────┘              └────────┬─────────┘
           │                               │
           v                               v
    (gui_app recibe)                (gui_app envía)
    Visualización en vivo           Controles de usuario
```

#### 3.5 Flujos de Datos Detallados

**Flujo de Envío de Datos a GUI:**
```cpp
HiloTransmisor (cada 20ms @ 50Hz)
    │
    ├─ Lee {vars->ref, vars->u, vars->yk, tiempo} con lock(vars.mtx)
    │
    └─ Transmisor::enviar()
         │
         ├─ Serializa en DataMessage (57 bytes, sin padding)
         │
         └─ mq_send() a /data_queue (no-bloqueante)
              │
              └─ GUI (gui_app) recibe bloqueante en hilo de comunicación
                   │
                   └─ Visualiza en gráficos en tiempo real
```

**Flujo de Cambio de Parámetros:**
```cpp
GUI (usuario ajusta Kp slider)
    │
    └─ Construye ParamsMessage (Kp_nuevo, Ki, Kd, setpoint)
         │
         └─ mq_send() a /params_queue
              │
              └─ HiloReceptor recibe en simulador
                   │
                   ├─ Receptor::recibir() deserializa
                   │
                   └─ Escribe en ParametrosCompartidos con lock(params.mtx)
                        │
                        └─ HiloPID lee parámetros actualizados cada ciclo
                             │
                             └─ Próximo ciclo usa kp_nuevo
```

#### 3.6 Serialización Manual sin Padding

```cpp
// comm.cpp implementa serialización manual
// Evita padding de compilador para portabilidad entre procesos

struct DataMessage {
    double values[6];           // 6 * 8 = 48 bytes
    double timestamp;           // 8 bytes
    uint8_t num_values;         // 1 byte
    // Total: 57 bytes (sin gaps)
};

struct ParamsMessage {
    double Kp, Ki, Kd;          // 3 * 8 = 24 bytes
    double setpoint;            // 8 bytes
    uint8_t signal_type;        // 1 byte
    uint32_t timestamp;         // 4 bytes
    // Total: 37 bytes
};
```

**Serialización (escribir en buffer):**
```cpp
size_t offset = 0;
memcpy(buffer + offset, &msg.Kp, sizeof(double)); offset += sizeof(double);
memcpy(buffer + offset, &msg.Ki, sizeof(double)); offset += sizeof(double);
// ... continuar para cada campo
```

**Deserialización (leer de buffer):**
```cpp
size_t offset = 0;
memcpy(&msg.Kp, buffer + offset, sizeof(double)); offset += sizeof(double);
// ... espejo de serialización
```

#### 3.7 Protocolo de Comunicación

**Queue Names:**
- `/data_queue`: Datos del simulador → GUI (muestreo continuo)
- `/params_queue`: Parámetros de GUI → Simulador (eventos discretos)

**Propiedades:**
- **mq_send()** (desde simulador): No-bloqueante, descarta si cola llena
- **mq_receive()** (en GUI): Bloqueante, espera próximo mensaje
- **Tamaño de cola**: Típicamente 10-50 mensajes
- **Prioridad**: Modo FIFO (First In First Out)

```cpp
┌────────────────────────────────────────┐
│     Control Simulator Process          │
│                                        │
│  ┌──────────┐      ┌──────────────┐  │
│  │ Setpoint │      │   Sumador    │  │
│  │ Generator├─────►│ (ref - y)    │  │
│  └──────────┘      └──────┬───────┘  │
│                           │ error     │
│                           ▼           │
│                    ┌──────────────┐  │
│                    │     PID      │  │
│                    │ Controller   │  │
│                    └──────┬───────┘  │
│                           │ u(t)     │
│                           ▼           │
│                    ┌──────────────┐  │
│                    │    Planta    │  │
│                    │ (TF o SS)    │  │
│                    └──────┬───────┘  │
│                           │ y(t)     │
│                           └──────────┤
│                                      │
│  Envía muestras via IPC ────────────►│
│  Recibe parámetros via IPC ◄─────────│
└────────────────────────────────────────┘
```

## 🔄 Flujos de Datos Principales

### Flujo de Control (Loop Cerrado)

1. **Generación de Setpoint**: `SignalGenerator` produce referencia
2. **Cálculo de Error**: `Sumador` calcula `e(k) = ref - y`
3. **Acción de Control**: `PIDController` genera `u(k)`
4. **Actualización de Planta**: `TransferFunctionSystem` produce `y(k)`
5. **Almacenamiento**: Todas las muestras se guardan en buffers

### Flujo de Parámetros

1. **ParamsMessage** recibido via `/params_queue`
2. **Simulador deserializa** el mensaje
3. **PID actualizado** con `setGains(Kp, Ki, Kd)`
4. **Control continúa** con nuevos parámetros

## 🧵 Modelo de Concurrencia

### Simulator Process

```
Main Thread
    │
    ├── pthread: HiloSignal (generador setpoint) @ 1000 Hz
    │
    ├── pthread: Hilo2in (sumador) @ 1000 Hz
    │
    ├── pthread: Hilo (PID) @ 1000 Hz
    │
    ├── pthread: Hilo (planta) @ 1000 Hz
    │
    └── Loop: Recepción de parámetros (blocking mq_receive)
```

Todos los hilos comparten variables protegidas por **un solo mutex global**.

### Modelos de Aplicación

```
GUI/Aplicación del Profesor
└── Main Thread (o Qt Event Loop)
    │
    └── Loop: Lectura de datos IPC
        │
        └── Procesamiento de datos recibidos
```

## 🛡️ Patrones de Diseño

### 1. Non-Virtual Interface (NVI)

```cpp
class DiscreteSystem {
public:
    double next(double uk) {        // Público, NO virtual
        double yk = compute(uk);    // Llama a virtual protegido
        storeSample(uk, yk);        // Garantiza almacenamiento
        return yk;
    }
    
protected:
    virtual double compute(double uk) = 0;  // Virtual puro protegido
};
```

**Beneficio**: Garantiza que todas las subclases almacenan muestras correctamente.

### 2. Template Method

`DiscreteSystem::next()` define el esqueleto del algoritmo; subclases implementan pasos específicos (`compute`).

### 3. Strategy

`SignalGenerator` permite intercambiar generadores sin cambiar código cliente:

```cpp
std::shared_ptr<Signal> sig1 = std::make_shared<SineSignal>(...);
std::shared_ptr<Signal> sig2 = std::make_shared<StepSignal>(...);
// Mismo interfaz, comportamiento diferente
```

### 4. RAII (Resource Acquisition Is Initialization)

```cpp
class Hilo {
public:
    Hilo(...) {
        pthread_create(&thread_, ...);  // Adquiere recurso
    }
    
    ~Hilo() {
        pthread_join(thread_, nullptr);  // Libera recurso
    }
};
```

### 5. Dependency Injection

Hilos reciben punteros a sistemas discretos, permitiendo testabilidad y flexibilidad:

```cpp
DiscreteSystem* system = ...; // Puede ser PID, TF, SS, etc.
Hilo hilo(system, ...);
```

## 📊 Gestión de Memoria

### Smart Pointers

```cpp
// Señales: shared_ptr para composición
auto signal = std::make_shared<SineSignal>(Ts, amp, freq);

// Sistemas: unique_ptr cuando ownership es único
std::unique_ptr<DiscreteSystem> pid(new PIDController(...));
```

### Buffers Circulares

```cpp
// DiscreteSystem usa índices manuales
size_t writeIndex_;
std::vector<Sample> buffer_;

// SignalGenerator usa std::deque
std::deque<double> value_buffer_;
```

**Beneficio**: Sin asignaciones dinámicas en hot loops.

### Sin malloc/free

Todo el proyecto usa contenedores STL y smart pointers exclusivamente.

## 🔐 Sincronización

### Patrón de Sincronización

```cpp
// Variables compartidas
double input_, output_;
std::mutex mtx_;

// Escritura
{
    std::lock_guard<std::mutex> lock(mtx_);
    output_ = nueva_salida;
}

// Lectura
{
    std::lock_guard<std::mutex> lock(mtx_);
    double val = input_;
}
```

### Frecuencia de Ejecución

```cpp
void Hilo::run() {
    int sleep_us = static_cast<int>(1e6 / frequency_);
    while (*running_) {
        // Trabajo...
        usleep(sleep_us);  // Espera período
    }
}
```

**Nota**: No es hard real-time. Para aplicaciones críticas, usar scheduler RT de Linux.

## 🧪 Testabilidad

### Inyección de Dependencias

```cpp
// Test puede inyectar mock
class MockPlant : public DiscreteSystem { ... };
MockPlant mock;
Hilo hilo(&mock, ...);
```

### Auto-descubrimiento de Tests

CMake busca `test/*.cpp` y crea ejecutables automáticamente:

```cmake
file(GLOB TEST_SOURCES "${CMAKE_SOURCE_DIR}/test/*.cpp")
foreach(TEST_SRC ${TEST_SOURCES})
    get_filename_component(EXE_NAME ${TEST_SRC} NAME_WE)
    add_executable(${EXE_NAME} ${TEST_SRC})
endforeach()
```

## 📈 Performance

### Hot Loop Optimizations

- **Buffer circular**: Sin allocations en `next()`
- **Inline methods**: Getters triviales inline
- **Lock scope mínimo**: Mutex solo en secciones críticas
- **Cálculos pre-computados**: Coeficientes PID calculados una vez

### Compilación Optimizada

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
# Flags: -O3 -march=native
```

## 🔍 Debugging

### Logs de Serialización

```cpp
// comm.cpp incluye logs en serialización
size_t serializeDataMessage(const DataMessage& msg, uint8_t* buffer) {
    // Debug: imprimir secuencia
    std::cout << "Serializing seq=" << msg.sequence << std::endl;
}
```

### Verificación de IPC

```bash
# Ver colas activas
ls -la /dev/mqueue/

# Test independiente
./Interfaz_Control/bin/test_send &
./Interfaz_Control/bin/test_receive
```

## 📚 Referencias Arquitectónicas

- **Patrón NVI**: Herb Sutter, "Virtuality"
- **RAII**: Bjarne Stroustrup, "The C++ Programming Language"
- **Threading**: POSIX Threads Programming, LLNL Tutorial
- **IPC**: "Advanced Programming in the UNIX Environment", Stevens
- **Qt Architecture**: Qt Documentation, Model-View-Controller

---

Para más detalles de implementación, consulta:
- [README.md](README.md) - Uso general
- [CONTRIBUTING.md](CONTRIBUTING.md) - Guías de desarrollo
- Documentación Doxygen - API completa
