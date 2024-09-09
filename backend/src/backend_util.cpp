#include <sstream>
#include <iomanip>
#include "../include/backend_util.h"
#include "../../frontend/include/LLVMIR.h"
#include "../../frontend/include/IRStatement.h"

namespace SCNUCC {

auto GetIEEE754(float num) -> std::string {
  float x = num;
  unsigned long *int_val = reinterpret_cast<unsigned long*>(&x);
  std::stringstream ss;
  ss << std::hex << std::setfill('0') << std::setw(16) << *int_val;
  return "0x" + ss.str();
}

} // namespace