/**
 * 辅助函数
 */
#ifndef SCNUCC_BACKEND_UTIL_H
#define SCNUCC_BACKEND_UTIL_H
#include "../../frontend/include/LLVMIR.h"
#include <memory>

namespace SCNUCC {

using namespace SCNUCC;
class IRStatement;
using IRPtr = std::shared_ptr<IRStatement>;
/** 
 *
 * 返回浮点数的16进制IEEE 754编码
 * @param float num
 * @return std::string
 */
auto GetIEEE754(float num) -> std::string;


}

#endif // SCNUCC_BACKEND_UTIL_H