#include <iostream>
#include "StateSpaceSystem.h"

void printOctave() {
    double tau = 0.3;

    // Comando Octave: carga control, define sistema, genera step y guarda en PNG
    std::string cmd = 
        "octave --silent --no-gui --eval \""
        "pkg load control; "
        "tau=" + std::to_string(tau) + "; "
        "s=tf('s'); "
        "G=1/(tau*s+1); "
        "step(G); "
        "print -dpng '../test/step.png';"
        "\"";

    std::cout << "Ejecutando Octave...\n";
    int ret = system(cmd.c_str());

}




int main() {
    using namespace DiscreteSystems;

    std::cout << "TEST ESPACIO DE ESTADOS" << std::endl;

    // Sistema de ejemplo en espacio de estados (1º orden)
    std::vector<std::vector<double>> A = {{0.99}};
    std::vector<double> B = {0.00995};
    std::vector<double> C = {1.0};
    double D = 0.0;
    double Ts = 0.01;
    size_t bufferSize = 10;

    StateSpaceSystem sys(A, B, C, D, Ts, bufferSize);

    

    std::cout << "Sampling Time: " << sys.getSamplingTime() << std::endl;
    std::cout << "Initial k: " << sys.getK() << std::endl;
    std::cout << "Initial Count: " << sys.getCount() << std::endl;

    std::cout << "Ahora llamamos a Next()" << std::endl;

    // Primera llamada con entrada 1 para escalón
    double Yk = sys.next(1);
    std::cout << "k: " << sys.getK() << " Yk: " << Yk << std::endl;

    // Simular varias muestras
    for(int i = 0; i < 20; i++) {
        Yk = sys.next(1);  // Escalón unitario
        std::cout << "k: " << sys.getK() << " Yk: " << Yk << std::endl;
    }

    // Mostrar buffer circular completo
    std::cout << std::endl<<"Buffer circular (últimas " << sys.getCount() << " muestras)"<<std::endl;

    //ejecutar en consola ../bin/testSS > ../test/output.txt (sobreescribe output.txt)
    //ejecutar en consola ../bin/testSS >> ../test/output.txt (añade salida a output.txt)

    return 0;
}
