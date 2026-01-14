# PL7 - Control de Sistemas Discretos en Tiempo Real
**Trabajo Final para Sistemas en Tiempo Real 2025-2026**

## 📋 Descripción General

Framework C++17 de control de sistemas en tiempo real para trabajo académico, implementando:
- **Librería Core**: Sistemas discretos reutilizables (PID, funciones de transferencia, generadores de señal)
- **Interfaz Gráfica**: GUI Qt6 con visualización en tiempo real y comunicación IPC
- **Threading**: Ejecución de sistemas en hilos POSIX con temporización absoluta

**Versión Actual**: v1.0.4 (Smart Pointers - Completada ✅)  
**Próxima**: v1.0.5 (Mejoras de robustez - En planificación)

---

## 🎯 Estado de Desarrollo

### v1.0.4 - Smart Pointers Migration (COMPLETADA ✅)
✅ Fase 1: Core threading (Hilo, Hilo2in, HiloPID)  
✅ Fase 2: Specialized threading (HiloSignal, HiloSwitch, HiloIntArranque, HiloTransmisor, HiloReceptor)  
✅ Fase 3: Client code (testHilo.cpp refactorizado)  
✅ Compilación 100% exitosa  
✅ Runtime validado  

**Cambios**: Migración total de punteros crudos a `shared_ptr` para ciclo de vida seguro

---

## 🚀 Quick Start

### Build
```bash
cd /home/jordi/PLs/PL7
./Interfaz_Control/build.sh   # Build completo (core + GUI)
# O manualmente:
cd build && cmake .. && make
```

### Tests
```bash
./bin/testHilo              # Test principal con hilos
./bin/testPID               # Test PID
./bin/testTF                # Test función de transferencia
# ... más tests disponibles en bin/
```

### Ejecutar GUI
```bash
./Interfaz_Control/bin/control_simulator &  # Simulador en background
./Interfaz_Control/bin/gui_app              # GUI (se conecta via IPC)
```

---

## 📁 Estructura de Proyecto

```
PL7/
├── include/                    # Headers (.h) organizados temáticamente
│   ├── hilos/                 # Clases de threading (Hilo*.h)
│   ├── sistemas/              # Sistemas discretos (PIDController.h, TF, SS)
│   ├── senales/               # Generadores de señal
│   ├── converters/            # Conversores AD/DA
│   ├── io/                    # Comunicación (Transmisor, Receptor)
│   ├── utilidades/            # Componentes auxiliares
│   └── config/                # Configuración centralizada
├── src/                        # Implementación (.cpp) con misma estructura
│   ├── hilos/
│   ├── sistemas/
│   ├── senales/
│   ├── converters/
│   ├── io/
│   ├── utilidades/
│   └── config/
├── test/                       # Tests unitarios
│   ├── testHilo.cpp           # Test integración completa
│   ├── testPID.cpp
│   └── ...
├── Interfaz_Control/          # Subsistema Qt6 (IPC, GUI)
│   ├── src/
│   └── build.sh
├── doc/                        # Documentación
│   ├── ASSESSMENT.md          # Evaluación del proyecto
│   ├── ARCHITECTURE.md        # Arquitectura detallada
│   ├── CHANGELOG.md           # Historial de versiones
│   ├── v1.0.5-ROADMAP.md     # Plan de mejoras futuras
│   └── ...
└── CMakeLists.txt            # Build CMake
```

---

## 📚 Documentación

- **[ARCHITECTURE.md](doc/ARCHITECTURE.md)**: Arquitectura general, flujos de datos, threading model
- **[ASSESSMENT.md](doc/ASSESSMENT.md)**: Fortalezas, debilidades e historial de mejoras
- **[CHANGELOG.md](doc/CHANGELOG.md)**: Historial completo de cambios por versión
- **[v1.0.5-ROADMAP.md](doc/v1.0.5-ROADMAP.md)**: Plan detallado para v1.0.5 (mejoras de robustez)
- **Doxygen**: Documentación de API en `doc/doxygen/html/`

---

## 🔧 Características Principales

### Core Library (libDiscreteSystems.a)
- **DiscreteSystem**: Clase base NVI para todos los sistemas
- **PIDController**: Control PID adaptativo
- **TransferFunctionSystem**: Función de transferencia discreta
- **StateSpaceSystem**: Representación en espacio de estados
- **SignalGenerator**: Composición de señales (paso, seno, PWM)
- **ADConverter/DAConverter**: Conversión analógico-digital

### Threading (v1.0.4 - Smart Pointers)
- **Hilo**: Base para ejecución periódica de sistema discreto
- **Hilo2in**: Variante para 2 entradas (ej. Sumador)
- **HiloPID**: Lectura dinámica de parámetros
- **HiloSignal, HiloSwitch**: Generadores de señal
- **HiloIntArranque, HiloTransmisor, HiloReceptor**: Especialización

**Mejora v1.0.4**: Ciclo de vida seguro mediante `shared_ptr` (co-ownership)

### Temporización
- **Temporizador**: Retardo absoluto con `clock_nanosleep(TIMER_ABSTIME)` para evitar drift
- **Discretizer**: Conversión planta continua → discreta (método Tustin)

### IPC (Interfaz ↔ Simulador)
- **Transmisor/Receptor**: Comunicación via POSIX message queues
- **Messages**: Serialización manual de DataMessage / ParamsMessage
- Desacoplamiento de GUI y lógica de control

---

## 📊 Validación

### Compilación
- ✅ libDiscreteSystems.a: Compila sin errores
- ✅ testHilo, testPID, testTF, etc.: 100% exitosos
- ✅ Interfaz_Control (Qt6): Compila correctamente

### Runtime
- ✅ testHilo ejecutado: Hilos funcionando, sincronización correcta
- ✅ Salidas: Variables compartidas actualizadas en tiempo real
- ✅ Frecuencias: Temporización absoluta validada (sin drift acumulativo)

---

## 🛠️ Tecnologías

- **C++17**: Estándar moderno, smart pointers, RAII
- **CMake**: Build system multiplataforma
- **POSIX Threads**: pthread (Linux/Unix)
- **Qt6**: GUI (subsistema Interfaz_Control)
- **Doxygen**: Documentación de API

---

## 📝 Cambios Recientes (v1.0.4)

### Migración a Smart Pointers (Completada)
- **Antes**: Punteros crudos → Riesgos de ciclo de vida
- **Ahora**: `shared_ptr` → Co-ownership explícita, ciclo de vida garantizado

**Ejemplo**:
```cpp
// v1.0.3 (punteros crudos)
Hilo hilo(&sistema, &entrada, &salida, &running, &mtx, freq);

// v1.0.4 (shared_ptr - nuevo)
auto sistema = std::make_shared<TransferFunctionSystem>(...);
auto entrada = std::make_shared<double>(0.0);
auto running = std::make_shared<bool>(true);
auto mtx = std::make_shared<pthread_mutex_t>();
Hilo hilo(sistema, entrada, salida, running, mtx, freq);
```

### Beneficios
✅ Eliminación de memory leaks  
✅ Acceso seguro entre threads  
✅ Propiedad clara  
✅ Compilación y runtime validados  

---

## 🚀 Próximos Pasos (v1.0.5)

Ver [doc/v1.0.5-ROADMAP.md](doc/v1.0.5-ROADMAP.md) para:
1. ✏️ Control de errores en threading (pthread_create/join)
2. 📝 Logging básico con timestamp
3. ⚙️ Configuración centralizada (Config.h)
4. 🔒 Separación de mutex por variable
5. ⏱️ Configuración de scheduler FIFO/RR

---

## 📞 Información del Proyecto

- **Autor**: Jordi
- **Asistencia**: GitHub Copilot
- **Asignatura**: Sistemas en Tiempo Real 2025-2026
- **Universidad**: [Universidad correspondiente]
- **Licencia**: [Especificar si aplica]

---

## 📖 Referencias

- [Documentación de PIDController](doc/mainpage.md)
- [Arquitectura de Sistemas Discretos](doc/ARCHITECTURE.md)
- [Evaluación del Proyecto](doc/ASSESSMENT.md)
- [C++ Reference - Smart Pointers](https://cplusplus.com/reference/memory/shared_ptr/)
- [POSIX Threads](https://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_create.html)
