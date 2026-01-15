
# Guía de Contribución

¡Gracias por tu interés en contribuir a PL7 Control de Sistemas Discretos! Este documento proporciona directrices para contribuir al proyecto.


## 📋 Tabla de Contenidos
    - [Ejecución de Tests](#ejecución-de-tests)
    - [Documentación y Diagramas](#documentación-y-diagramas)
    - [Archivos de Configuración Clave](#archivos-de-configuración-clave)
## 🧪 Ejecución de Tests

Todos los tests unitarios se encuentran en la carpeta `test/` y se auto-descubren con CMake. Para compilar y ejecutar los tests:

```bash
cd build
cmake .. && make
./bin/testPID         # Ejecuta el test de PID
./bin/testTF          # Ejecuta el test de función de transferencia
# ...otros binarios en ./bin/
```

Los resultados pueden guardarse en archivos CSV/TSV en el directorio `test/`.

## 📖 Documentación y Diagramas

La documentación completa de la API está generada con Doxygen. Consulta los diagramas y descripciones en:
- `doc/ARCHITECTURE.md` (arquitectura y diagramas de flujo)
- `doc/mainpage.md` (resumen general)
- Documentación HTML generada por Doxygen en `doc/doxygen/html/index.html`

## 🛠️ Archivos de Configuración Clave

- `include/config/system_config.h`: configuración centralizada (SSOT)
- `CMakeLists.txt`: configuración de build y tests

## 📝 Estándar de Código

- C++17 obligatorio en todo el proyecto
- Usa `clang-format` para mantener el estilo consistente (opcional pero recomendado)
- Sigue los patrones de diseño modernos (RAII, NVI, smart pointers)


- [Código de Conducta](#código-de-conducta)
- [¿Cómo puedo contribuir?](#cómo-puedo-contribuir)
- [Proceso de Desarrollo](#proceso-de-desarrollo)
- [Estándares de Código](#estándares-de-código)
- [Commit Guidelines](#commit-guidelines)
- [Testing](#testing)
- [Documentación](#documentación)

## 📜 Código de Conducta

Este es un proyecto académico (Trabajo Final). Se espera que todos los participantes:

- Sean respetuosos y constructivos
- Acepten críticas constructivas
- Se enfoquen en lo mejor para la comunidad educativa
- Muestren empatía hacia otros colaboradores

**Nota**: Este es un trabajo final académico desarrollado por Jordi con asistencia de GitHub Copilot.

## 🤝 ¿Cómo puedo contribuir?

### Reportar Bugs

Los bugs se rastrean como issues de GitHub. Al crear un bug report, incluye:

- **Título claro y descriptivo**
- **Pasos para reproducir** el problema
- **Comportamiento esperado** vs **comportamiento actual**
- **Versión** del compilador y CMake
- **Sistema operativo**
- **Logs o capturas** relevantes

**Ejemplo:**
```markdown
## Bug: Segmentation fault en PIDController

**Descripción:** El programa crashea al ejecutar testPID con Kp=0

**Pasos para reproducir:**
1. Compilar con `cmake .. && make`
2. Ejecutar `./bin/testPID`
3. Modificar Kp a 0.0

**Esperado:** Error de validación o comportamiento definido
**Actual:** Segmentation fault

**Entorno:**
- GCC 11.3.0
- Ubuntu 22.04
- CMake 3.22.1
```

### Sugerir Mejoras

Las sugerencias de features son bienvenidas. Incluye:

- **Descripción detallada** de la funcionalidad
- **Caso de uso educativo** (por qué es útil para el curso)
- **Ejemplos de código** de cómo se usaría
- **Impacto** en la arquitectura existente

### Pull Requests

1. **Fork** el repositorio
2. **Crea una rama** desde `main`:
   ```bash
   git checkout -b feature/nombre-descriptivo
   ```
3. **Haz tus cambios** siguiendo los estándares de código
4. **Añade tests** si aplica
5. **Actualiza documentación** (README, Doxygen)
6. **Commit** siguiendo las convenciones
7. **Push** a tu fork
8. **Abre un Pull Request**

## 🔧 Proceso de Desarrollo

### Configurar Entorno de Desarrollo

```bash
# Clonar tu fork
git clone https://github.com/TU_USUARIO/PL7.git
cd PL7

# Añadir upstream
git remote add upstream https://github.com/REPO_ORIGINAL/PL7.git

# Instalar dependencias
sudo apt-get install build-essential cmake doxygen graphviz clang-format

# Compilar
mkdir build && cd build
cmake ..
make
```

### Workflow de Desarrollo

```bash
# Actualizar tu fork
git checkout main
git pull upstream main

# Crear rama para feature
git checkout -b feature/mi-feature

# Hacer cambios
# ...

# Compilar y testear
cd build
make
./bin/testTuFeature

# Commit
git add .
git commit -m "feat: añadir nueva funcionalidad X"

# Push
git push origin feature/mi-feature
```

## 🌟 Arquitectura y Patrones Clave

### Entender el Diseño IPC

El proyecto utiliza comunicación inter-procesos (IPC) con POSIX message queues:

```cpp
// Simulador (control_simulator)
ParametrosCompartidos params;       // Thread-safe con mutex POSIX
VariablesCompartidas vars;

HiloReceptor rx(&receptor, &running, &mtx, 50);    // Recibe parámetros
HiloTransmisor tx(&transmisor, &running, &mtx, 50); // Envía datos

// GUI (gui_app)
// Envía ParamsMessage a /params_queue
// Recibe DataMessage de /data_queue
```

### Patrones Implementados

1. **Non-Virtual Interface (NVI)**: `DiscreteSystem::next()` garantiza almacenamiento
2. **Strategy Pattern**: `SignalGenerator` permite intercambiar señales
3. **Dependency Injection**: Hilos reciben sistemas como parámetros
4. **RAII**: Threads automáticamente joined en destructor
5. **Template Method**: Clase base define flujo, subclases implementan detalles

### Guía para Extender Componentes IPC

Si añades nuevo componente de comunicación:

```cpp
// include/MiComponente.h
class MiComponente {
public:
    bool inicializar();     // Conecta a mqueue
    bool enviar();          // O recibir() según sea
    void cerrar();          // Desconecta
    
private:
    std::unique_ptr<MQueueComm> comm_;
    bool inicializado_;
};

// Crear HiloMiComponente para threading periódico
class HiloMiComponente {
public:
    HiloMiComponente(MiComponente* comp, bool* running,
                     pthread_mutex_t* mtx, double frequency);
    ~HiloMiComponente();
    
private:
    static void* threadFunc(void* arg);
    void run();
};
```

## 📝 Estándares de Código para IPC

### C++17

```cpp
// ✅ CORRECTO: Usa estándar moderno
auto signal = std::make_shared<SignalGenerator::SineSignal>(Ts, amp, freq);
std::lock_guard<std::mutex> lock(mtx);

// ❌ INCORRECTO: Evita malloc/free manual
double* buffer = (double*)malloc(100 * sizeof(double));
free(buffer);
```

### Nomenclatura

```cpp
// Clases: PascalCase
class TransferFunctionSystem { };

// Funciones/métodos: camelCase
double computeOutput(double input);

// Variables: camelCase o snake_case
double samplingTime;
double sampling_time;  // También aceptable

// Constantes: UPPER_SNAKE_CASE (preferiblemente en system_config.h)
const int MAX_BUFFER_SIZE = 1000;

// Miembros privados: terminan en _
class MyClass {
private:
    double value_;
    std::mutex mtx_;
};
```

### Configuración Centralizada

**IMPORTANTE**: Todas las frecuencias, períodos y tamaños de buffer deben definirse en `include/config/system_config.h`:

```cpp
// ✅ CORRECTO: Usar constantes de system_config.h
#include "config/system_config.h"
double ts = SystemConfig::TS_CONTROLLER;
double freq = SystemConfig::FREQ_COMMUNICATION;
size_t buffer_size = SystemConfig::BUFFER_SIZE_LOGGER;

// ❌ INCORRECTO: Hardcodear valores
double ts = 0.01;  // NO - usar SystemConfig::TS_CONTROLLER
double freq = 50.0;  // NO - usar SystemConfig::FREQ_COMMUNICATION
```

**Single Source of Truth (SSOT)**: `config/system_config.h` es el único lugar para definir configuración del sistema.

### Añadir un Nuevo Componente

**Paso 1: Crear header en carpeta temática**
```bash
# Ejemplo: Nuevo sistema discreto
touch include/sistemas/MyNewSystem.h
```

**Paso 2: Crear implementación en carpeta temática**
```bash
touch src/sistemas/MyNewSystem.cpp
```

**Paso 3: Usar includes con rutas relativas**
```cpp
// En include/sistemas/MyNewSystem.h
#include "sistemas/DiscreteSystem.h"

// En src/sistemas/MyNewSystem.cpp
#include "sistemas/MyNewSystem.h"
```

**Paso 4: Crear test**
```bash
touch test/testMyNewSystem.cpp
```

**Paso 5: CMake detectará automáticamente**
- Los archivos .cpp en `src/**/*.cpp` se compilarán automáticamente
- Los tests en `test/*.cpp` se crearán como ejecutables

### Patrón NVI

```cpp
// ✅ CORRECTO: Clase base sigue patrón NVI
class DiscreteSystem {
public:
    double next(double uk) {  // Público, no-virtual
        double yk = compute(uk);
        storeSample(uk, yk);
        return yk;
    }
    
protected:
    virtual double compute(double uk) = 0;  // Protegido, virtual puro
};

// Subclase sobrescribe solo compute()
class PIDController : public DiscreteSystem {
protected:
    double compute(double ek) override {
        // Implementación
    }
};
```

### Sincronización Segura con Mutex POSIX

```cpp
// ✅ CORRECTO: Proteger acceso a ParametrosCompartidos
{
    std::lock_guard<pthread_mutex_t> lock(params->mtx);
    double current_kp = params->kp;
    params->kp = new_value;
}

// ❌ INCORRECTO: Acceso sin protección
double kp = params->kp;  // Carrera de datos posible
```

### Serialización Manual IPC

```cpp
// ✅ CORRECTO: Sin padding (portable)
struct DataMessage {
    double values[6];       // 48 bytes
    double timestamp;       // 8 bytes
    uint8_t num_values;     // 1 byte
    // Total: 57 bytes exacto
};

// ❌ INCORRECTO: Con padding implícito
struct BadMessage {
    uint8_t flag;           // 1 byte
    double value;           // 8 bytes (padding: 7 bytes!)
    // Total: 16 bytes (se pierden 7 bytes)
};
```

### Comentarios Doxygen

```cpp
/**
 * @brief Descripción breve de la función
 * 
 * Descripción detallada con más información sobre el comportamiento,
 * algoritmos utilizados, y consideraciones especiales.
 * 
 * @param input Descripción del parámetro de entrada
 * @param frequency Frecuencia en Hz (debe ser > 0)
 * @return Descripción del valor de retorno
 * @throws std::runtime_error si frequency <= 0
 * 
 * @note Consideraciones especiales o warnings
 * @see RelatedFunction(), RelatedClass
 * 
 * Ejemplo de uso:
 * @code{.cpp}
 * double result = myFunction(5.0, 100.0);
 * @endcode
 */
double myFunction(double input, double frequency);
```

### Formato de Código

Usa `clang-format` con el estilo del proyecto:

```bash
# Formatear archivo individual
clang-format -i src/MiArchivo.cpp

# Formatear todos los archivos
find src/ include/ -name '*.cpp' -o -name '*.h' | xargs clang-format -i
```

## 📋 Commit Guidelines

Seguimos [Conventional Commits](https://www.conventionalcommits.org/):

```
<tipo>(<scope>): <descripción>

[cuerpo opcional]

[footer opcional]
```

### Tipos de Commit

- `feat`: Nueva funcionalidad
- `fix`: Corrección de bug
- `docs`: Cambios solo en documentación
- `style`: Formato (no afecta código)
- `refactor`: Refactorización sin cambio funcional
- `perf`: Mejora de rendimiento
- `test`: Añadir o modificar tests
- `build`: Cambios en sistema de build (CMake)
- `ci`: Cambios en CI/CD
- `chore`: Tareas de mantenimiento

### Ejemplos

```bash
feat(pid): añadir método setGains() para sintonización eficiente

fix(transfer-function): corregir normalización de coeficientes

docs(readme): actualizar instrucciones de instalación para Ubuntu 24.04

refactor(hilo): simplificar lógica de sincronización de mutex

test(signal-generator): añadir test para PWM signal

build(cmake): actualizar versión mínima a 3.16
```

## 🧪 Testing

### Crear un Nuevo Test

```cpp
// test/testMiFeature.cpp
#include "../include/MiFeature.h"
#include <iostream>
#include <cassert>

int main() {
    // Setup
    DiscreteSystems::MiFeature feature(params);
    
    // Test
    double result = feature.compute(input);
    
    // Verify
    assert(std::abs(result - expected) < 1e-6);
    
    std::cout << "Test passed!" << std::endl;
    return 0;
}
```

### Testing de Componentes IPC

Para probar nuevos componentes IPC (Receptor, Transmisor, etc.):

```cpp
// test/testMiReceptor.cpp
#include "../include/Receptor.h"
#include "../include/ParametrosCompartidos.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    ParametrosCompartidos params;
    Receptor receptor(&params);
    
    // Inicializar comunicación
    if (!receptor.inicializar()) {
        std::cerr << "Failed to initialize receiver" << std::endl;
        return 1;
    }
    
    // Test: recibir mensaje (requiere envío desde otro proceso)
    // std::thread sender(testSendMessage);
    // receptor.recibir();
    // sender.join();
    
    receptor.cerrar();
    std::cout << "IPC test passed!" << std::endl;
    return 0;
}
```

### Verificar Funcionamiento de IPC

```bash
# Terminal 1: Test receptor
./Interfaz_Control/bin/test_receive

# Terminal 2: Test transmisor
./Interfaz_Control/bin/test_send
```

### Ejecutar Tests

```bash
# Recompilar con tests
cd build
cmake ..
make

# Ejecutar todos los tests
for test in ../bin/test*; do
    echo "Running $test..."
    $test
done

# O ejecutar test específico
./bin/testMiFeature
```

### Cobertura de Tests

Asegúrate de que tu código esté cubierto:
- Casos normales de uso
- Casos extremos (edge cases)
- Condiciones de error
- Valores límite

## 📚 Documentación

### Actualizar README

Si tu cambio afecta el uso del proyecto:
- Actualiza sección relevante en `README.md`
- Añade ejemplos de código si aplica
- Actualiza tabla de características

### Comentarios Doxygen

Todo código público debe tener comentarios Doxygen:

```cpp
/**
 * @file MiArchivo.h
 * @brief Descripción breve del archivo
 * @author Tu Nombre
 * @date 2024-12-18
 */

/**
 * @class MiClase
 * @brief Descripción de la clase
 * 
 * Descripción detallada del propósito, uso y comportamiento.
 * 
 * @invariant condición que siempre debe cumplirse
 */
```

### Generar y Verificar Documentación

```bash
# Regenerar Doxygen
doxygen Doxyfile

# Verificar que no hay warnings
doxygen Doxyfile 2>&1 | grep -i warning

# Abrir en navegador
xdg-open doc/doxygen/html/index.html
```

## ✅ Checklist Pre-Pull Request

Antes de enviar tu PR, verifica:

- [ ] El código compila sin warnings (`-Wall -Wextra`)
- [ ] Todos los tests pasan
- [ ] Añadiste tests para nueva funcionalidad
- [ ] Documentación Doxygen actualizada
- [ ] README actualizado si aplica
- [ ] Commits siguen convenciones
- [ ] Código formateado con `clang-format`
- [ ] No hay conflictos con `main`

## 🎓 Filosofía de Contribución

Este es un **Trabajo Final Académico** (Trabajo Final de Sistemas en Tiempo Real). Al contribuir, considera:

- **Claridad pedagógica**: El código enseña buenas prácticas, no solo resuelve problemas
- **Comentarios exhaustivos**: Explica el "por qué" especialmente en temas avanzados (threading, IPC)
- **Patrones demostrativos**: Usa patrones de diseño que sean educativos
- **Testing completo**: Los tests sirven como ejemplos de uso
- **Documentación abundante**: Doxygen comments para API pública
- **Ejemplos prácticos**: Incluye ejemplos de uso reales en comentarios
- **Considera futuras iteraciones**: El código debe ser extensible para asignaturas posteriores

### Consideraciones Especiales para Componentes de Tiempo Real

Si contribuyes código de threading o IPC:

1. **Documen ta el patrón de sincronización**: Explica por qué se usa ese mutex
2. **Describe posibles deadlocks**: Aunque sea uno solo, menciona cómo evitarlo
3. **Discute tradeoffs**: ¿Qué ganas y qué pierdes con este diseño?
4. **Proporciona ejemplos thread-safe**: Muestra cómo usar la clase de forma segura
5. **Test bajo contención**: Verifica que funciona con múltiples threads

Ejemplo de contribución educativa:

```cpp
/**
 * @brief Sincronización de acceso a parámetros compartidos
 * 
 * Este método demuestra el patrón RAII con std::lock_guard para
 * garantizar la liberación del mutex incluso si se lanza excepción.
 * 
 * @param kp Nueva ganancia proporcional
 * 
 * @note Patrón pedagógico: muestra cómo evitar deadlocks
 * @warning Si se mantiene el lock durante cálculos, se reducirá paralelismo
 * 
 * @code{.cpp}
 * {
 *     std::lock_guard<pthread_mutex_t> lock(params->mtx);
 *     params->kp = 1.5;  // Acceso seguro
 * }  // Lock liberado automáticamente aquí
 * @endcode
 */
void setKp(double kp);
```

## 💬 ¿Preguntas?

Si tienes dudas sobre cómo contribuir:
- Abre un issue con la etiqueta `question`
- Revisa issues anteriores
- Consulta la documentación Doxygen

---

¡Gracias por contribuir a PL7! 🚀
