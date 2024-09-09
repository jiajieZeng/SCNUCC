//
//  
//
#include "../include/IRVariable.h"
#include "../include/IRStatement.h"

namespace SCNUCC {
auto IRVariable::IsGbl() -> bool {
  return std::dynamic_pointer_cast<GblVarDeclIR>(defined_in_) != nullptr;
}
}