# Guía de Contribución

¡Gracias por tu interés en contribuir a PL7 Control de Sistemas Discretos! Este documento proporciona directrices para contribuir al proyecto.

## 📋 Tabla de Contenidos

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

## 📝 Estándares de Código

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

// Constantes: UPPER_SNAKE_CASE
const int MAX_BUFFER_SIZE = 1000;

// Miembros privados: terminan en _
class MyClass {
private:
    double value_;
    std::mutex mtx_;
};
```

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

### Smart Pointers

```cpp
// ✅ CORRECTO: Usa smart pointers
auto signal = std::make_shared<Signal>(Ts);
std::unique_ptr<DiscreteSystem> system(new PIDController(Kp, Ki, Kd, Ts));

// ❌ INCORRECTO: Evita punteros crudos para ownership
Signal* signal = new Signal(Ts);
delete signal;  // Propenso a memory leaks
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

## 🎓 Filosofía del Pracadémico** (Trabajo Final de Sistemas en Tiempo Real). Al contribuir, considera:

- **Claridad sobre complejidad**: El código debe ser profesional pero comprensible
- **Mejores prácticas**: Aplicación de patrones de diseño y buenas prácticas de C++17
- **Claridad sobre complejidad**: El código debe ser entendible para estudiantes
- **Patrones pedagógicos**: Usa patrones que enseñen buenas prácticas
- **Documentación exhaustiva**: Explica el "por qué", no solo el "qué"
- **Ejemplos prácticos**: Incluye ejemplos de uso reales

## 💬 ¿Preguntas?

Si tienes dudas sobre cómo contribuir:
- Abre un issue con la etiqueta `question`
- Revisa issues anteriores
- Consulta la documentación Doxygen

---

¡Gracias por contribuir a PL7! 🚀
