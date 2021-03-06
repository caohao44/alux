#ifndef __ALUX_X64_PROGRAM_IMAGE_HPP__
#define __ALUX_X64_PROGRAM_IMAGE_HPP__

#include <anarch/stddef>
#include <anarch/types>

namespace Alux {

namespace x64 {

class ProgramImage {
public:
  ProgramImage();
  
  inline PhysAddr GetKernelEnd() const {
    return kernelEnd;
  }
  
  inline PhysAddr GetProgramStart() const {
    return programStart;
  }
  
  inline size_t GetProgramSize() const {
    return programSize;
  }
  
  inline PhysAddr GetProgramEnd() const {
    return programStart + programSize;
  }
  
private:
  PhysAddr kernelEnd;
  PhysAddr programStart;
  size_t programSize;
};

}

}

#endif
