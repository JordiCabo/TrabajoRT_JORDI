/**
 * @file DiscreteSystem.h
 * @brief Clase base abstracta para sistemas discretos SISO
 * @author Jordi + GitHub Copilot
 * @date 2025-10-26
 */

#ifndef DISCRETESYSTEMS_DISCRETESYSTEM_H
#define DISCRETESYSTEMS_DISCRETESYSTEM_H

#include <vector>
#include <iostream>
#include <string>

namespace DiscreteSystems {

/**
 * @struct Sample
 * @brief Representa una muestra del sistema con entrada, salida y paso temporal
 */
struct Sample {
    double in;    ///< Valor de entrada u(k)
    double out;   ///< Valor de salida y(k)
    int k;        ///< Índice temporal (paso k)
};

/**
 * @enum ExportFormat
 * @brief Formato de exportación del buffer de muestras
 */
enum class ExportFormat {
    TSV,      ///< Tab-Separated Values (compatible con MATLAB/Octave)
    MATLAB    ///< Formato MATLAB con espacios y sintaxis load()
};

/**
 * @class DiscreteSystem
 * @brief Clase base abstracta para sistemas discretos SISO
 * 
 * Implementa el patrón NVI (Non-Virtual Interface) para garantizar
 * que todas las muestras se almacenan correctamente en el buffer
 * circular independientemente de la implementación concreta.
 * 
 * @invariant 0 <= count_ <= bufferSize_
 * @invariant 0 <= writeIndex_ < bufferSize_
 * @invariant k_ >= 0
 */
class DiscreteSystem {
public:
    /**
     * @brief Constructor
     * @param Ts Período de muestreo (debe ser > 0)
     * @param bufferSize Tamaño del buffer circular (por defecto 100)
     * @throws InvalidSamplingTime si Ts <= 0
     */
    DiscreteSystem(double Ts, size_t bufferSize = 100);

    /**
     * @brief Destructor virtual
     */
    virtual ~DiscreteSystem() = default;

    /**
     * @brief Calcula la siguiente salida del sistema (método NVI)
     * 
     * Este método público no es virtual. Garantiza que:
     * 1. Se llama a compute(uk) para calcular la salida
     * 2. La muestra se almacena en el buffer mediante storeSample()
     * 3. El índice temporal k_ se incrementa
     * 
     * @param uk Entrada en el paso k
     * @return Salida y(k)
     */
    double next(double uk);
    double next (double in1, double in2); 

    /**
     * @brief Reinicia el sistema al estado inicial
     * 
     * Método no virtual que:
     * 1. Reinicia k_ a 0
     * 2. Vacía el buffer (count_ = 0, writeIndex_ = 0)
     * 3. Llama al hook virtual resetState() para que las clases
     *    derivadas reinicien su estado interno
     */
    void reset();

    // Export del buffer (deshabilitado). Si se reactiva, documentar aquí sin @param para evitar warnings Doxygen.
    // void bufferDump(std::ostream& os, ExportFormat format = ExportFormat::TSV) const;

    /**
     * @brief Obtiene el período de muestreo
     * @return Período de muestreo Ts
     */
    double getSamplingTime() const { return Ts_; }

    /**
     * @brief Obtiene el índice temporal actual
     * @return Paso temporal k
     */
    int getK() const { return k_; }

    /**
     * @brief Obtiene el número de muestras válidas en el buffer
     * @return Número de muestras almacenadas (0 <= count <= bufferSize)
     */
    size_t getCount() const { return count_; }

protected:
    /**
     * @brief Calcula la salida del sistema (método virtual puro)
     * 
     * Las clases derivadas deben implementar este método para
     * definir la ecuación en diferencias o el modelo de espacio
     * de estados correspondiente.
     * 
     * @param uk Entrada en el paso k
     * @return Salida y(k)
     */
    virtual double compute(double uk) = 0;

    //para bloques de 2 entradas, no obligatorio implementar en clases derivadas, se usa compute de 2 entradas
    virtual double compute(double in1, double in2){return 0.0;}; 

    /**
     * @brief Reinicia el estado interno del sistema (hook virtual)
     * 
     * Las clases derivadas deben implementar este método para
     * reiniciar sus variables de estado (historiales, estado interno, etc.)
     */
    virtual void resetState() = 0;

private:
    /**
     * @brief Almacena una muestra en el buffer circular
     * @param uk Entrada
     * @param yk Salida
     * 
     * Implementa la lógica de buffer circular:
     * - Si count_ < bufferSize_: almacena en writeIndex_ e incrementa count_
     * - Si count_ == bufferSize_: sobrescribe la muestra más antigua
     */
    void storeSample(double uk, double yk);

    double Ts_;                      ///< Período de muestreo
    int k_;                          ///< Índice temporal actual
    size_t bufferSize_;              ///< Tamaño del buffer
    size_t writeIndex_;              ///< Índice de escritura en el buffer circular
    size_t count_;                   ///< Número de muestras válidas (0 <= count_ <= bufferSize_)
    std::vector<Sample> buffer_;     ///< Buffer circular de muestras
};

} // namespace DiscreteSystems

#endif // DISCRETESYSTEMS_DISCRETESYSTEM_H
