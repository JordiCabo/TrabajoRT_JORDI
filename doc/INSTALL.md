# Guía de Instalación

Guía detallada de instalación para PL7 Control de Sistemas Discretos en diferentes plataformas.

## 📋 Tabla de Contenidos

- [Requisitos del Sistema](#requisitos-del-sistema)
- [Ubuntu/Debian](#ubuntudebian)
- [Arch Linux](#arch-linux)
- [Fedora/RHEL](#fedorarhel)
- [macOS](#macos)
- [Verificación de Instalación](#verificación-de-instalación)
- [Troubleshooting](#troubleshooting)

## 💻 Requisitos del Sistema

### Hardware Mínimo
- **Procesador**: x86_64 con soporte para C++17
- **RAM**: 2 GB mínimo, 4 GB recomendado
- **Disco**: 500 MB para código fuente y compilación

### Software Requerido
- **Compilador C++**: GCC >= 7.0 o Clang >= 5.0 con soporte C++17
- **CMake**: >= 3.10
- **pthread**: Soporte POSIX threads (incluido en sistemas Unix)
- **rt**: POSIX real-time extensions (message queues)

### Software Opcional
- **Doxygen**: >= 1.8.13 (para documentación)
- **Graphviz**: Para diagramas en documentación
- **Git**: Para control de versiones
- **clang-format**: Para formato de código

## 🐧 Ubuntu/Debian

### Ubuntu 22.04 LTS / 24.04 LTS

```bash
# Actualizar repositorios
sudo apt-get update

# Instalar dependencias de compilación
sudo apt-get install -y build-essential cmake git

# Instalar herramientas de documentación (opcional)
sudo apt-get install -y doxygen graphviz

# Verificar instalación
g++ --version      # Debe ser >= 7.0
cmake --version    # Debe ser >= 3.10
```

### Debian 11/12

```bash
# Habilitar repositorios backports si es necesario
echo "deb http://deb.debian.org/debian $(lsb_release -sc)-backports main" | \
    sudo tee /etc/apt/sources.list.d/backports.list

sudo apt-get update

# Instalar dependencias
sudo apt-get install -y build-essential cmake git
sudo apt-get install -y doxygen graphviz
```

## 🎯 Arch Linux

```bash
# Actualizar sistema
sudo pacman -Syu

# Instalar dependencias base
sudo pacman -S base-devel cmake git

# Instalar herramientas de documentación (opcional)
sudo pacman -S doxygen graphviz

# Verificar instalación
g++ --version
cmake --version
```

## 🎩 Fedora/RHEL

### Fedora 38+

```bash
# Actualizar sistema
sudo dnf update

# Instalar dependencias de compilación
sudo dnf install -y gcc-c++ cmake git

# Instalar Qt6
sudo dnf install -y qt6-qtbase-devel qt6-qtcharts-devel

# Instalar documentación (opcional)
sudo dnf install -y doxygen graphviz

# Verificar
g++ --version
cmake --version
```

### RHEL 8/9 / Rocky Linux

```bash
# Habilitar repositorios adicionales
sudo dnf install -y epel-release
sudo dnf config-manager --set-enabled powertools  # RHEL 8
# o
sudo dnf config-manager --set-enabled crb         # RHEL 9

# Instalar dependencias
sudo dnf groupinstall -y "Development Tools"
sudo dnf install -y cmake git
sudo dnf install -y qt6-qtbase-devel qt6-qtcharts-devel
sudo dnf install -y doxygen graphviz
```

## 🍎 macOS

### Con Homebrew

```bash
# Instalar Homebrew si no está instalado
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Instalar dependencias
brew install cmake git
brew install doxygen graphviz

# Verificar instalación
g++ --version
cmake --version
```

## 🔧 Compilación del Proyecto

Una vez instaladas las dependencias:

```bash
# Clonar el repositorio
git clone https://github.com/USUARIO/PL7.git
cd PL7

# Compilar librería core
mkdir -p build && cd build
cmake ..
make -j$(nproc)  # Compilación paralela
cd ..
```

## ✅ Verificación de Instalación

### Verificar Compilación

```bash
# Verificar que los ejecutables se crearon
ls -lh bin/

# Ejecutar un test simple
./bin/testPID
```

### Verificar Dependencias del Sistema

```bash
# POSIX message queues
ls -la /dev/mqueue/

# pthread
ldconfig -p | grep pthread

# rt (real-time)
ldconfig -p | grep librt
```

### Ejecutar Tests Completos

```bash
# Ejecutar todos los tests
cd bin
for test in test*; do
    echo "=== Ejecutando $test ==="
    ./$test
done
```

### Generar Documentación

```bash
# Desde el directorio raíz
doxygen Doxyfile

# Verificar que se generó
ls -lh doc/doxygen/html/index.html

# Abrir en navegador
xdg-open doc/doxygen/html/index.html  # Linux
open doc/doxygen/html/index.html      # macOS
```

## 🐛 Troubleshooting

### Error: "Qt6 not found"

**Ubuntu/Debian:**
```bash
# Instalar paquetes de compilación
sudo apt-get install -y build-essential cmake git
sudo apt-get install -y doxygen graphviz
```

**Otros sistemas:**
```bash
# Verifica que tienes los compiladores básicos
g++ --version
cmake --version
```

### Error: "CMake version too old"

```bash
# Ubuntu: Instalar desde backports
sudo apt-get install -y -t $(lsb_release -sc)-backports cmake

# O instalar desde snap
sudo snap install cmake --classic

# Verificar versión
cmake --version
```

### Error: "undefined reference to pthread_create"

Añade flag de pthread al CMakeLists.txt:
```cmake
target_link_libraries(tu_ejecutable PRIVATE pthread)
```

### Error: "/dev/mqueue/ not available"

```bash
# Montar filesystem de message queues
sudo mkdir -p /dev/mqueue
sudo mount -t mqueue none /dev/mqueue

# Hacer permanente (añadir a /etc/fstab)
echo "none /dev/mqueue mqueue defaults 0 0" | sudo tee -a /etc/fstab
```

### Warning: "Command line option '-std=c++17' is valid for C++/ObjC++"

Tu compilador es muy antiguo. Actualiza:

**Ubuntu:**
```bash
sudo apt-get install -y gcc-11 g++-11
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 100
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 100
```

### Problemas con Graphviz

```bash
# Verificar instalación
dot -V

# Si no está instalado
sudo apt-get install -y graphviz  # Ubuntu/Debian
sudo pacman -S graphviz            # Arch
brew install graphviz              # macOS
```

## 📞 Soporte Adicional

Si sigues teniendo problemas:

1. **Revisa los logs de compilación**: `cmake .. 2>&1 | tee cmake.log`
2. **Verifica versiones**: Asegúrate de cumplir versiones mínimas
3. **Limpia build**: `rm -rf build/ && mkdir build`
4. **Consulta issues**: Revisa problemas similares en el repositorio
5. **Abre un issue**: Proporciona logs completos y versiones

## 🔄 Actualización

Para actualizar a una nueva versión:

```bash
# Actualizar código
git pull origin main

# Limpiar builds antiguos
rm -rf build/ Interfaz_Control/build/

# Recompilar
mkdir -p build && cd build
cmake ..
make -j$(nproc)
cd ..
```

---

¿Instalación exitosa? Continúa con el [README](README.md) para ejemplos de uso.
