
#include "DiscreteSystem.h"

namespace DiscreteSystems {

    DiscreteSystem::DiscreteSystem(double Ts, size_t bufferSize):
     Ts_(Ts),
      k_(0),
      bufferSize_(bufferSize),
      writeIndex_(0),
      count_(0),
      buffer_(bufferSize)   
{
    if (Ts <= 0) throw std::runtime_error("InvalidSamplingTime: Ts must be > 0");
    
    std::cout << "Objeto de tipo DiscreteSystem creado correctamente" << std::endl;
}
    
        
    double DiscreteSystem::next(double uk){
     
    double yk = compute(uk);
    storeSample(uk, yk);
    int currentK = k_;
    k_++;
    return yk;

    }

    double DiscreteSystem::next(double in1, double in2){
     
    double yk = compute(in1, in2);
    storeSample(in1, yk); //no necesito almacenar la referencia, es solo para el sumador este bloque
    int currentK = k_;
    k_++;
    return yk;

    }
    
    void DiscreteSystem::reset(){
        std::cout << "Reset ejecutado" << std::endl; 
    }
   
    //void bufferDump(std::ostream& os, ExportFormat format = ExportFormat::TSV) const;

    void DiscreteSystem::storeSample(double uk, double yk){
       

          // Guardar la muestra en la posición actual del buffer circular
    buffer_[writeIndex_] = Sample{ uk, yk, k_ };

    // Si aún no está lleno, incrementa count_
    if (count_ < bufferSize_)
        count_++;

    // Mover índice circular
    writeIndex_ = (writeIndex_ + 1) % bufferSize_;
    }


} // namespace DiscreteSystems