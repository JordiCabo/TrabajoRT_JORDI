/**
 * @file Discretizer.cpp
 * @brief Implementación de discretización de funciones de transferencia continuas.
 */

#include "../include/Discretizer.h"
#include <cmath>

namespace DiscreteSystems {

// --- Helpers de polinomios (en variable x = z^-1) ---
static std::vector<double> polyMul(const std::vector<double>& a, const std::vector<double>& b) {
    std::vector<double> r(a.size() + b.size() - 1, 0.0);
    for (size_t i = 0; i < a.size(); ++i) {
        for (size_t j = 0; j < b.size(); ++j) {
            r[i + j] += a[i] * b[j];
        }
    }
    return r;
}

static std::vector<double> polyAdd(const std::vector<double>& a, const std::vector<double>& b) {
    size_t n = std::max(a.size(), b.size());
    std::vector<double> r(n, 0.0);
    for (size_t i = 0; i < n; ++i) {
        double av = (i < a.size()) ? a[i] : 0.0;
        double bv = (i < b.size()) ? b[i] : 0.0;
        r[i] = av + bv;
    }
    return r;
}

// (1 + sign*x)^k usando binomio de Newton. sign = +1 ó -1.
static std::vector<double> binomialPoly(int k, int sign) {
    std::vector<double> c(k + 1, 0.0);
    c[0] = 1.0;
    for (int i = 1; i <= k; ++i) {
        c[i] = c[i - 1] * (k - i + 1) / i;
    }
    if (sign == -1) {
        for (int i = 1; i <= k; ++i) {
            if (i % 2 == 1) c[i] = -c[i];
        }
    }
    return c; // coeficientes ascendentes en x^i
}

// Term transformado: c * (K*(1-x)/(1+x))^p * (1+x)^N = c*K^p*(1-x)^p*(1+x)^{N-p}
static std::vector<double> transformTerm(double c, int p, double K, int N) {
    if (p == 0) {
        // Solo multiplicar por (1+x)^N
        auto plusN = binomialPoly(N, +1);
        for (double &v : plusN) v *= c; // escalar
        return plusN;
    }
    auto oneMinus = binomialPoly(p, -1);        // (1 - x)^p
    auto onePlus  = binomialPoly(N - p, +1);    // (1 + x)^{N-p}
    auto term = polyMul(oneMinus, onePlus);
    double scale = c * std::pow(K, p);
    for (double &v : term) v *= scale;
    return term;
}

// Aplica bilinear a un polinomio en s y limpia denominadores con (1+x)^N
static std::vector<double> bilinearPoly(const std::vector<double>& coeffs, double Ts, int N) {
    int order = static_cast<int>(coeffs.size()) - 1; // grado en s
    double K = 2.0 / Ts; // sustitución s = K*(1 - x)/(1 + x)
    std::vector<double> acc{0.0};
    for (int i = 0; i <= order; ++i) {
        double c = coeffs[i];
        int p = order - i; // potencia de s
        auto term = transformTerm(c, p, K, N);
        acc = polyAdd(acc, term);
    }
    return acc; // coef en x^i (x = z^-1)
}

DiscreteTF discretizeTF(const std::vector<double>& num_s,
                        const std::vector<double>& den_s,
                        double Ts,
                        DiscretizationMethod method) {
    if (Ts <= 0.0) {
        throw std::invalid_argument("Ts debe ser > 0");
    }
    if (den_s.empty() || std::abs(den_s[0]) < 1e-12) {
        throw std::invalid_argument("Denominador continuo inválido");
    }

    switch (method) {
    case DiscretizationMethod::Tustin: {
        int na = static_cast<int>(den_s.size()) - 1;
        // Transformar numerador y denominador con el mismo N=na
        auto bd = bilinearPoly(num_s, Ts, na);
        auto ad = bilinearPoly(den_s, Ts, na);

        // Normalizar para que a[0] = 1
        double a0 = ad.front();
        for (double &v : bd) v /= a0;
        for (double &v : ad) v /= a0;

        return DiscreteTF{bd, ad};
    }
    case DiscretizationMethod::ZOH:
        throw std::invalid_argument("Método ZOH no implementado aún");
    default:
        throw std::invalid_argument("Método de discretización no soportado");
    }
}

} // namespace DiscreteSystems
