#include "RISCVModel/RVM.hpp"
#include "RISCVModel/VTable.h"

void build_cpp() {
  rvm::RVM_FunctionPointers FP;
  RVMConfig Conf;
  rvm::State::Builder Builder(&FP);
  auto State = Builder.build();
}
