
/**
 * RISCV指令的枚举
 */

#ifndef SCNUCC_RISCV_H
#define SCNUCC_RISCV_H

#include <map>
#include <string>

namespace SCNUCC {
// https://pku-minic.github.io/online-doc/#/misc-app-ref/riscv-insts
enum RegisterName {  // 整数寄存器
    ZERO, // 这个寄存器连接到零，总是为零
    RA, // 存储返回地址，在执行RET语句的时候自动跳转到这个地址，CALLER-SAVED
    SP, // 栈指针，增长方向是向下的，即从高地址向低地址，CALLEE-SAVED
    GP, // 全局指针，暂时不知道用来干嘛
    TP, // 线程指针，暂时也不知道用来干嘛
    T0, T1, T2, T3, T4, T5, T6, // 临时寄存器，CALLER-SAVED
    S0, S1, S2, S3, S4, S5, S6, S7, S8, S9, S10, S11, // 保存寄存器，CALLEE-SAVED
    A0, A1,  // 参数/返回值寄存器，CALLER-SAVED
    A2, A3, A4, A5, A6, A7,  // 参数寄存器，CALLER-SAVED

    FT0, FT1, FT2, FT3, FT4, FT5, FT6, FT7, FT8, FT9, FT10, FT11, // 浮点寄存器，CALLER-SAVED
    FS0, FS1, FS2, FS3, FS4, FS5, FS6, FS7, FS8, FS9, FS10, FS11, // 浮点寄存器，CALLEE-SAVED
    FA0, FA1, FA2, FA3, FA4, FA5, FA6, FA7, // 浮点寄存器，CALLER-SAVED

    NONE, // 表示没有寄存器
};

enum AsmInstructionType {
  // 指令的具体作用参考这里：https://pku-minic.github.io/online-doc/#/misc-app-ref/riscv-insts
  // 控制转移类指令
  JUMP, // 无条件跳转
  BEQZ, // 等于0时跳转
  BNEZ, // 不等于0时跳转
  CALL, // 函数调用
  RET, // 函数返回

  // 访存类指令
  LW, // 加载
  SW, // 存储

  // 运算类
  ADD, // 加法
  ADDI, // 加法立即数
  SUB, // 减法
  SLT, // 小于
  SGT, // 大于
  SEQZ,
  SNEZ,
  XOR,
  XORI,
  OR,
  ORI,
  AND,
  ANDI,
  SLL,
  SRL,
  SRA,
  MUL, DIV, REM,

  SEQ,
  SNE,
  SLE,
  SGE,
  // 浮点数
  FADD,
  FSUB,
  FDIV,
  FMUL,
  FLT,
  FGT,
  FEQZ,
  FNEZ,
  FEQ,
  FNE,
  FLE,
  FGE,
  
  // 加载移动类
  LI, // 加载立即数
  MV, // 移动
  LA, // 加载地址
  LUI, // 加载高20位

  BLOCKDECL, // 声明一个新的基本块
  // 类型转换
  FTOI, // float to int
  ITOF, // int to float
  BTOI, // bool to int
  MITOF, // move int to float register

  SLLI,
  SLTU,
  COMMENT,
};

extern std::map<RegisterName, std::string> reg2str;

extern std::map<AsmInstructionType, std::string> isn2str;

} // namespace SCNUCC

#endif //SCNUCC_RISCV_H
