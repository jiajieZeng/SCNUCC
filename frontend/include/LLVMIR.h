//
//  .
//

#ifndef SCNUCC_LLVMIR_H
#define SCNUCC_LLVMIR_H


#include <string>
#include <unordered_map>
#include <any>


namespace SCNUCC {

namespace IROP {
enum IROP {
    // 整数运算符
    IR_ADD,
    IR_SUB,
    IR_MUL,
    IR_SDIV,  // signed division，sysY2022没有unsigned division
    IR_SREM,  // 有符号数取余
    // 浮点数运算符
    IR_FADD,
    IR_FSUB,
    IR_FMUL,
    IR_FDIV,
    IR_FREM, // 浮点数取余
    // 整数的比较运算符
    IR_IGT,
    IR_IGE,
    IR_ILT,
    IR_ILE,
    IR_IEQ,
    IR_INE,
    // 浮点数的比较运算符
    IR_FGT,
    IR_FGE,
    IR_FLT,
    IR_FLE,
    IR_FEQ, // TODO：什么情况用OEQ？什么时候用UEQ？
    IR_FNE,
    // 类型转换
    IR_ITOF,  // int to float
    IR_FTOI, // float to int
    IR_BTOI, // bool to int
    // 单目运算符
    IR_NEG,  // 取负
    // 逻辑运算符
    IR_AND,
    IR_NOT,
    IR_OR,
    IR_XOR,
    // 内存操作
    IR_ALLOCA,
    IR_STR,
    IR_LRD,
    // 分支
    IR_BR,
    IR_RET,
    IR_CALL,
    // phi函数
    IR_PHI,
    IR_BLOCKDECL,
    IR_PLACEHOLDER,  // 用于占位, 无实际意义
    // 指针转换，memset数组的时候用
    IR_BITCAST,
    // 获取指针
    IR_GEP,
}; // enum IR_OP
} // namespace IROP

extern std::unordered_map<IROP::IROP, std::string> IR_OP_STR; /*= {
        {IROP::IR_ADD, "add"},  i32
        {IROP::IR_SUB, "sub"},
        {IROP::IR_MUL, "mul"},
        {IROP::IR_SDIV, "sdiv"},
        {IROP::IR_SREM, "srem"},
        {IROP::IR_ITOF, "sitofp"},
        {IROP::IR_FADD, "fadd"},    float
        {IROP::IR_FSUB, "fsub"},
        {IROP::IR_FMUL, "fmul"},
        {IROP::IR_FDIV, "fdiv"},
        {IROP::IR_FREM, "frem"},
        {IROP::IR_FTOI, "fptosi"},
        {IROP::IR_IGT, "icmp sgt"}, i1
        {IROP::IR_IGE, "icmp sge"},
        {IROP::IR_ILT, "icmp slt"},
        {IROP::IR_ILE, "icmp sle"},
        {IROP::IR_IEQ, "icmp eq"},
        {IROP::IR_INE, "icmp ne"},
        {IROP::IR_FGT, "fcmp ogt"},
        {IROP::IR_FGE, "fcmp oge"},
        {IROP::IR_FLT, "fcmp olt"},
        {IROP::IR_FLE, "fcmp ole"},
        {IROP::IR_FEQ, "fcmp oeq"},
        {IROP::IR_FNE, "fcmp one"},
        {IROP::IR_NEG, "neg"},
        {IROP::IR_AND, "and"},
        {IROP::IR_NOT, "not"},
        {IROP::IR_OR, "or"},
        {IROP::IR_ALLOCA, "alloca"},
        {IROP::IR_STR, "store"},
        {IROP::IR_LRD, "load"},
        {IROP::IR_BR, "br"},
        {IROP::IR_RET, "ret"},
        {IROP::IR_CALL, "call"},
        {IROP::IR_PHI, "phi"},
        {IROP::IR_BTOI, "zext"}
}; // IR_OP_STR*/

namespace IRTYPE {
enum IRTYPE {
    IR_VOID, IR_I1, IR_I32, IR_I64, IR_FLOAT, IR_DOUBLE, IR_PLACEHOLDER,
    /* 常量 */
    IR_CONST_INT, IR_CONST_FLOAT, IR_CONST_BOOL,
    /* 指针 */
    IR_INT_PTR, IR_FLOAT_PTR
}; // enum IR_TYPE
} // namespace IRTYPE

extern std::unordered_map<IRTYPE::IRTYPE, std::string> IR_TYPE_STR;/* = {
        {IRTYPE::IR_I1, "i1"},
        {IRTYPE::IR_I32, "i32"},
        {IRTYPE::IR_I64, "i64"},
        {IRTYPE::IR_FLOAT, "float"},
        {IRTYPE::IR_DOUBLE, "double"}
}; // IR_TYPE_STR*/

} // namespace SCNUCC

#endif //SCNUCC_LLVMIR_H
