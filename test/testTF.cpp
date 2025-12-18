#include <iostream>
#include "TransferFunctionSystem.h"

void printOctave() {

    std::vector<double> b = {0.00995};       // Numerador
    std::vector<double> a = {1.0, -0.99};    // Denominador
    double Ts = 0.01;

    // Convertir vectores a strings separados por comas
    std::string b_str = "";
    for(size_t i=0; i<b.size(); ++i) {
        b_str += std::to_string(b[i]);
        if(i != b.size()-1) b_str += ",";
    }

    std::string a_str = "";
    for(size_t i=0; i<a.size(); ++i) {
        a_str += std::to_string(a[i]);
        if(i != a.size()-1) a_str += ",";
    }

    // Comando Octave: sistema discreto y guardar gráfico
    std::string cmd = 
   
    "octave --silent --no-gui --eval '"
    "pkg load control; "
    "b=[" + b_str + "]; "
    "a=[" + a_str + "]; "
    "Ts=" + std::to_string(Ts) + "; "
    "Gd=tf(b,a,Ts); "
    "[y, t] = step(Gd, 20); "
    "csvwrite(\"../test/step_values.csv\", [t,y]); "
    "step(Gd, 20); "
    "print -dpng \"../test/step_discreto.png\";'";

    std::cout << "Ejecutando Octave...\n";
    int ret = system(cmd.c_str());

}

int main() {
    using namespace DiscreteSystems;

    std::cout << "TEST FUNCION DE TRANSFERENCIA" << std::endl;

    // Sistema de ejemplo: 1º orden discreto
    std::vector<double> b = {0.00995};        // Numerador
    std::vector<double> a = {1.0, -0.99};     // Denominador
    double Ts = 0.01;
    size_t bufferSize = 10;

    TransferFunctionSystem Gz(b, a, Ts, bufferSize);

    std::cout << "Sampling Time: " << Gz.getSamplingTime() << std::endl;
    std::cout << "Initial k: " << Gz.getK() << std::endl;
    std::cout << "Initial Count: " << Gz.getCount() << std::endl;

    std::cout << "Ahora llamamos a Next()" << std::endl;

    // Primera llamada con entrada 1 para escalón
    double Yk = Gz.next(1);
    std::cout << "k: " << Gz.getK() << " Yk: " << Yk << std::endl;

    // Simular varias muestras
    for(int i = 0; i < 20; i++) {
        Yk = Gz.next(1);  // Escalón unitario
        std::cout << "k: " << Gz.getK() << " Yk: " << Yk << std::endl;
    }

    // Mostrar buffer circular completo
    std::cout << "\nBuffer circular (últimas " << Gz.getCount() << " muestras)" << std::endl;
   

    printOctave();
      //ejecutar en consola ../bin/testSS > ../test/output.txt (sobreescribe output.txt)
    //ejecutar en consola ../bin/testSS >> ../test/output.txt (añade salida a output.txt)

    return 0;
}
