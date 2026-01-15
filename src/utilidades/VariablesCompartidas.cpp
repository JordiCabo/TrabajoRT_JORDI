#include "utilidades/VariablesCompartidas.h"
#include <fstream>

VariablesCompartidas::VariablesCompartidas()
    : ref(0.0), e(0.0), u(0.0), ua(0.0), yk(0.0), ykd(0.0), running(false)
{
    // Inicializar mutex POSIX
    pthread_mutex_init(&mtx, nullptr);
    
    // Pre-reservar espacio en buffer para evitar realocaciones
    buffer_muestras.reserve(MAX_MUESTRAS);
}

VariablesCompartidas::~VariablesCompartidas()
{
    // Destruir mutex POSIX
    pthread_mutex_destroy(&mtx);
}

/**
 * @brief Sobrecarga del operador << para impresión formateada
 * 
 * Imprime el estado actual de todas las variables del lazo en un formato legible:
 * Ref=X | e=X | u=X | yk=X | ykd=X
 * 
 * @note NO protege automáticamente con mutex. El llamante es responsable de sincronización.
 *       Para uso seguro en multihilo, proteger como:
 *       pthread_mutex_lock(&vars.mtx);
 *       std::cout << vars << std::endl;
 *       pthread_mutex_unlock(&vars.mtx);
 */
std::ostream& operator<<(std::ostream& os, const VariablesCompartidas& vars) {
    os << "Ref=" << vars.ref
       << " | e=" << vars.e
       << " | u=" << vars.u
       << " | yk=" << vars.yk
       << " | ykd=" << vars.ykd;
    return os;
}

void VariablesCompartidas::guardarMuestraEnBuffer(int k, const std::string& linea) {
    // Guardar solo si hay espacio disponible (no bloquea, O(1))
    if (buffer_muestras.size() < MAX_MUESTRAS) {
        buffer_muestras.push_back(linea);
    }
}

bool VariablesCompartidas::escribirBufferADisco(const std::string& filename) {
    std::ofstream archivo(filename, std::ios::app);
    if (!archivo.is_open()) {
        return false;
    }
    
    // Escribir el encabezado
    archivo << "=== Ejecución del Sistema ===" << std::endl;
    archivo << "===========================" << std::endl;
    
    // Escribir todas las muestras del buffer
    for (const auto& muestra : buffer_muestras) {
        archivo << muestra << std::endl;
    }
    
    archivo << "=== Fin de Ejecución ===" << std::endl;
    archivo.close();
    
    return true;
}

void VariablesCompartidas::limpiarBuffer() {
    buffer_muestras.clear();
}

size_t VariablesCompartidas::obtenerTamanoBuffer() const {
    return buffer_muestras.size();
}