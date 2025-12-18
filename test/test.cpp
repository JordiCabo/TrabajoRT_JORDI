#include <cstdlib>
#include <iostream>

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
   printOctave();
    return 0;
}