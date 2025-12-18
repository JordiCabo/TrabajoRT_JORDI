#pragma once

#include <vector>
#include <deque>
#include <memory>
#include <ostream>
#include <cstddef>

namespace SignalGenerator {

/**
 * @brief Clase base para señales temporizadas discretas.
 *
 * Proporciona la infraestructura común: periodo de muestreo, tiempo
 * actual, buffers circulares para tiempos y valores, y la interfaz pública
 * para calcular muestras y avanzar en el tiempo.
 *
 * Diseño y notas para estudiantes:
 * - Esta clase es abstracta: define la interfaz y deja la implementación
 *   matemática a las subclases a través de computeAt(time).
 * - Separa cálculo puro (computeAt/compute) de operaciones con efectos
 *   (next), lo que facilita pruebas y razonamiento.
 * - Usamos contenedores STL (std::deque) y smart pointers en subclases
 *   compuestas para evitar manejo manual de memoria (malloc/free).
 *
 * Ejemplo básico de uso:
 * @code{.cpp}
 * auto s = std::make_shared<SignalGenerator::SineSignal>(0.001, 1.0, 2.0);
 * double v0 = s->compute();       // valor en t=0 sin avanzar
 * double v1 = s->next();          // valor en t=0 y avanza t+=Ts
 * double sample_k = s->compute(10); // valor en la muestra k=10 sin avanzar
 * @endcode
 */
class Signal {
protected:
    double Ts_;             // Período de muestreo [s]
    double offset_;         // Desplazamiento vertical
    double t_;              // Tiempo actual [s]
    std::size_t buffer_size_;  // Tamaño máximo del buffer circular

    std::deque<double> time_buffer_;
    std::deque<double> value_buffer_;

    void addToBuffer(double time, double value);

public:
    /**
     * @brief Construye una señal.
     * @param Ts Periodo de muestreo en segundos (debe ser > 0).
     * @param offset Desplazamiento vertical que se suma a la salida.
     * @param buffer_size Tamaño máximo del buffer circular (>=1).
     * @throw std::invalid_argument si Ts <= 0 o buffer_size == 0.
     */
    explicit Signal(double Ts, double offset = 0.0, std::size_t buffer_size = 1024);

    /**
     * @brief Destructor virtual por seguridad al usar polimorfismo.
     */
    virtual ~Signal() = default;

    /**
     * @brief Calcula el valor de la señal en el tiempo interno actual (t_).
     *
     * No modifica el estado del objeto (método const). Equivalente a
     * computeAt(t_).
     * @return Valor de la señal en el tiempo actual.
     */
    double compute() const;

    /**
     * @brief Calcula el valor de la señal en la muestra k (sin efectos).
     *
     * No modifica el estado del objeto. El tiempo usado para el cálculo es
     * time = k * Ts_.
     * @param k Índice de la muestra (0-based).
     * @return Valor de la señal en la muestra k.
     */
    double compute(std::size_t k) const;

    /**
     * @section design_notes Diseño: compute vs next
     * - compute() y compute(k) son operaciones puras (no modifican estado).
     * - next() es la operación que realiza el muestreo: guarda en buffers y
     *   avanza el tiempo interno. Esta separación ayuda a que los tests puedan
     *   verificar valores sin provocar efectos secundarios.
     */

    /**
     * @brief Calcula la siguiente muestra, la almacena en el buffer y avanza el tiempo.
     *
     * Llama internamente a compute() para obtener el valor en el tiempo actual,
     * guarda el par (time,value) en los buffers y hace t_ += Ts_.
     * @return Valor calculado para la muestra actual (antes de avanzar t_).
     */
    virtual double next();

    /**
     * @brief Reinicia la señal: pone t_ a 0 y limpia los buffers.
     */
    virtual void reset();

    /** @name Getters / Setters */
    ///@{
    double& T();
    const double& T() const;

    double& offset();
    const double& offset() const;

    double& t();
    const double& t() const;

    std::size_t& bufferSize();
    const std::size_t& bufferSize() const;

    const std::deque<double>& timeBuffer() const;
    const std::deque<double>& valueBuffer() const;
    ///@}

    /**
     * @brief Serializa el contenido de los buffers como CSV en el stream.
     * @param os Stream de salida.
     * @param s Señal a serializar.
     * @return Referencia al stream.
     */
    friend std::ostream& operator<<(std::ostream& os, const Signal& s);

protected:
    /**
     * @brief Método que debe implementar cada subclase: calcula el valor en time.
     * @param time Tiempo en segundos donde evaluar la señal.
     * @return Valor de la señal en el tiempo dado.
     */
    virtual double computeAt(double time) const = 0;
};

class StepSignal : public Signal {
    double amplitude_;
    double step_time_;

public:
    /**
     * @brief Construye una señal escalón.
     * @param Ts Periodo de muestreo [s].
     * @param amplitude Valor de la amplitud del escalón.
     * @param step_time Tiempo a partir del cual la señal toma la amplitud.
     * @param offset Desplazamiento vertical adicional.
     * @param buffer_size Tamaño del buffer.
     */
    StepSignal(double Ts, double amplitude, double step_time,
               double offset = 0.0, std::size_t buffer_size = 1024);

    /**
     * @brief Calcula el valor del escalón en un tiempo dado.
     * @param time Tiempo en segundos.
     */
    double computeAt(double time) const override;

    /**
     * @note Diseño: StepSignal ilustra una subclase concreta que sólo implementa
     * la fórmula matemática. No gestiona buffers; esa responsabilidad es de
     * la clase base.
     */

    double& amplitude();
    const double& amplitude() const;

    double& stepTime();
    const double& stepTime() const;
};

class PwmSignal : public Signal {
    double amplitude_;
    double duty_;
    double period_;

public:
    /**
     * @brief Construye una señal PWM.
     * @param Ts Periodo de muestreo [s].
     * @param amplitude Amplitud de pulso.
     * @param duty Ciclo de trabajo [0..1].
     * @param period Periodo de la PWM en segundos.
     * @param offset Desplazamiento vertical.
     * @param buffer_size Tamaño del buffer.
     */
    PwmSignal(double Ts, double amplitude, double duty, double period,
              double offset = 0.0, std::size_t buffer_size = 1024);

    /**
     * @brief Calcula el valor de la PWM en un tiempo dado.
     * @param time Tiempo en segundos.
     */
    double computeAt(double time) const override;

    /**
     * @note Diseño: PwmSignal muestra cómo encapsular parámetros (duty, period)
     * y validar en el constructor para evitar estados inválidos durante el uso.
     */

    double& amplitude();
    const double& amplitude() const;

    double& duty();
    const double& duty() const;

    double& period();
    const double& period() const;
};

class SineSignal : public Signal {
    double amplitude_;
    double freq_;
    double phase_;

public:
    /**
     * @brief Construye una señal sinusoidal.
     * @param Ts Periodo de muestreo [s].
     * @param amplitude Amplitud de la señal.
     * @param freq Frecuencia en Hz.
     * @param phase Fase inicial en radianes.
     * @param offset Desplazamiento vertical.
     * @param buffer_size Tamaño del buffer.
     */
    SineSignal(double Ts, double amplitude, double freq,
               double phase = 0.0, double offset = 0.0,
               std::size_t buffer_size = 1024);

    /**
     * @brief Calcula el valor seno en el tiempo dado.
     * @param time Tiempo en segundos.
     */
    double computeAt(double time) const override;

    /**
     * @note Diseño: SineSignal es un ejemplo de señal analítica; observar cómo
     * computeAt no modifica el estado, lo cual facilita el razonamiento.
     */

    double& amplitude();
    const double& amplitude() const;

    double& frequency();
    const double& frequency() const;

    double& phase();
    const double& phase() const;
};

class SignalMixer : public Signal {
    std::vector<std::shared_ptr<Signal>> signals_;
    std::vector<double> weights_;

public:
    SignalMixer(double Ts,
                std::vector<std::shared_ptr<Signal>> signals,
                std::vector<double> weights = {},
                double offset = 0.0,
                std::size_t buffer_size = 1024);

    /**
     * @brief Calcula el valor mezclado en un tiempo dado (sin efectos).
     * @param time Tiempo en segundos.
     * @return Suma ponderada de las salidas de las señales internas.
     *
     * @note Diseño: SignalMixer demuestra composición: contiene punteros a
     * otras señales y combina sus salidas. Usamos std::shared_ptr porque la
     * propiedad puede ser compartida por distintos componentes. Si la
     * propiedad fuera única, sería preferible std::unique_ptr.
     */
    double computeAt(double time) const override;

    std::vector<std::shared_ptr<Signal>>& signals();
    const std::vector<std::shared_ptr<Signal>>& signals() const;

    std::vector<double>& weights();
    const std::vector<double>& weights() const;

    /**
     * @brief Avanza todas las señales internas (llama a next() en cada una)
     * y almacena la mezcla en el buffer propio.
     * @return Valor de la mezcla en la muestra avanzada.
     */
    double next() override;
};

} // namespace SignalGenerator
