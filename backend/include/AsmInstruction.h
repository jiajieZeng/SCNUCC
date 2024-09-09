
/**
 * 定义一众的RISCV汇编指令，每一种指令都是AsmInstruction的子类
 */

#ifndef SCNUCC_ASMINSTRUCTION_H
#define SCNUCC_ASMINSTRUCTION_H

#include <string>
#include <memory>
#include <vector>
#include "./riscv.h"
#include "./Location.h"
#include "../../frontend/include/IRStatement.h"

namespace SCNUCC {
class IRVariable;

class IRStatement;

class Location;

class Register;

class Stack;

class Immediate12;

class Immediate20;

class Immediate32;

typedef std::shared_ptr<Register> RegisterPtr;

typedef std::shared_ptr<Stack> StackPtr;

typedef std::shared_ptr<Immediate12> Imm12Ptr;

typedef std::shared_ptr<Immediate20> Imm20Ptr;

typedef std::shared_ptr<Immediate32> Imm32Ptr;

typedef std::shared_ptr<IRVariable> IRVariablePtr;

class AsmInstruction {
public:
  AsmInstructionType type_;

  std::string comment_;

  AsmInstruction(AsmInstructionType type) : type_(type) {}

  virtual auto ToString() -> std::string = 0;
};

class Comment : public AsmInstruction {
public:
  Comment(const std::string &comment) : AsmInstruction(AsmInstructionType::COMMENT) {
    comment_ = "#" + comment;
  }

  auto ToString() -> std::string override {
    return "# " + comment_;
  }
};

class JumpInstruction : public AsmInstruction {
  std::string target_blk_name_;
public:
  JumpInstruction(std::string &blk_name) : AsmInstruction(AsmInstructionType::JUMP), target_blk_name_(blk_name) {
  }

  auto ToString() -> std::string override;
};

class BnezInstruction : public AsmInstruction {
  std::string target_blk_name_;
  RegisterPtr cond;
public:
  BnezInstruction(RegisterPtr &cond, std::string &blk_name) : AsmInstruction(BNEZ) {
    this->cond = cond;
    target_blk_name_ = blk_name;
  }

  auto ToString() -> std::string override;
};

class CallInstruction : public AsmInstruction {
  std::string func_name_;
  std::vector<Type> types_;
public:
  CallInstruction(const std::string &func_name, const std::vector<Type> &types) : AsmInstruction(
          AsmInstructionType::CALL) {
    func_name_ = func_name;
    types_ = types;
  }

  auto ToString() -> std::string override;
};

class RetInstruction : public AsmInstruction {
public:
  RetInstruction() : AsmInstruction(AsmInstructionType::RET) {}

  auto ToString() -> std::string override;
};

class LoadInstruction : public AsmInstruction {
  RegisterPtr target_;
  StackPtr stack_;

  RegisterName base_;
  int offset_;
  int load_float_;
public:
  LoadInstruction(const RegisterPtr &target, const StackPtr &stack, int load_float = 0) : AsmInstruction(AsmInstructionType::LW) {
    target_ = target;
    stack_ = stack;
    load_float_ = load_float;
  }

  LoadInstruction(const RegisterPtr &target, RegisterName base, int offset, int load_float = 0) : AsmInstruction(AsmInstructionType::LW) {
    target_ = target;
    base_ = base;
    offset_ = offset;
    load_float_ = load_float;
    stack_ = nullptr;
  }

  auto ToString() -> std::string override;
};

class StoreInstruction : public AsmInstruction {
  RegisterPtr src_;
  StackPtr target_;

  RegisterName base_;
  int offset_;
public:
  StoreInstruction(const StackPtr &target, const RegisterPtr &src) : AsmInstruction(
          AsmInstructionType::SW) {
    if (!target || !src) {
      throw std::runtime_error("StoreInstruction: nullptr");
    }
    src_ = src;
    target_ = target;
  }

  StoreInstruction(RegisterName base, int offset, const RegisterPtr &src) : AsmInstruction(AsmInstructionType::SW) {
    base_ = base;
    offset_ = offset;
    src_ = src;
  }

  auto ToString() -> std::string override;

};

class StoreArgsInstruction : public AsmInstruction {
  RegisterPtr src_;
  std::shared_ptr<ArgsStack> target_;
public:
  StoreArgsInstruction(const std::shared_ptr<ArgsStack> &target, const RegisterPtr &src) : AsmInstruction(
          AsmInstructionType::SW) {
    src_ = src;
    target_ = target;
  }

  auto ToString() -> std::string override;
};

class BinaryInstruction : public AsmInstruction {
  // 包括add, sub, slt, sgt, xor, or, and, sll, srl, sra, mul, div, rem
  RegisterPtr target_;
  RegisterPtr src1_;
  RegisterPtr src2_;
public:
  BinaryInstruction(AsmInstructionType type, const std::vector<RegisterPtr> &regs)
          : AsmInstruction(type) {
    target_ = regs[0];
    src1_ = regs[1];
    src2_ = regs[2];
  }

  BinaryInstruction(AsmInstructionType type, RegisterPtr &target, RegisterPtr &src1, RegisterPtr &src2)
          : AsmInstruction(
          type) {
    target_ = target;
    src1_ = src1;
    src2_ = src2;
  }

  auto ToString() -> std::string override;
};

/**
 * addi, xori, ori, andi
 */
class BinaryImmInstruction : public AsmInstruction {
  // BinaryInstruction的立即数版本
  RegisterPtr target_;
  RegisterPtr src_;
  Imm12Ptr imm_;
public:
  BinaryImmInstruction(AsmInstructionType type, RegisterPtr &target, RegisterPtr &src, Imm12Ptr &imm) : AsmInstruction(
          type) {
    target_ = target;
    src_ = src;
    imm_ = imm;
  }

  auto ToString() -> std::string override;

};

class LoadImmInstruction : public AsmInstruction {
  RegisterPtr target_;
  Imm32Ptr imm_;
public:
  LoadImmInstruction(RegisterPtr &target, Imm32Ptr &imm) : AsmInstruction(AsmInstructionType::LI) {
    target_ = target;
    imm_ = imm;
  }

  auto ToString() -> std::string override;
};

class LoadAddrInstruction : public AsmInstruction {
  RegisterPtr target_;
  std::string label_;
public:
  LoadAddrInstruction(RegisterPtr &target, std::string &label) : AsmInstruction(AsmInstructionType::LA) {
    target_ = target;
    label_ = label;
  }


  auto ToString() -> std::string override;
};

class MoveInstruction : public AsmInstruction {
  RegisterPtr rd_;
  RegisterPtr rs_;
public:
  MoveInstruction(RegisterPtr &rd, RegisterPtr &rs) : AsmInstruction(AsmInstructionType::MV) {
    if (!rd || !rs) {
      throw std::runtime_error("MoveInstruction: nullptr");
    }
    rd_ = rd;
    rs_ = rs;
  }

  auto ToString() -> std::string override;
};

class LoadUpperInstruction : public AsmInstruction {
  RegisterPtr target_;
  Imm12Ptr imm_;
public:
  LoadUpperInstruction(RegisterPtr &target, Imm12Ptr &imm) : AsmInstruction(AsmInstructionType::LUI) {
    target_ = target;
    imm_ = imm;
  }

  auto ToString() -> std::string override;
};

// fcvt.s.w rd, rs1：将整数寄存器 rs1 中的整数转换为单精度浮点数，并将结果存储到浮点寄存器 rd 中。
class IntToFloatInstruction : public AsmInstruction {
  RegisterPtr src_;
  RegisterPtr target_;
public:
  IntToFloatInstruction(RegisterPtr &src, RegisterPtr &target) : AsmInstruction(AsmInstructionType::ITOF) {
    src_ = src;
    target_ = target;
  }

  auto ToString() -> std::string override;
};

// fcvt.w.s rd, rs1：将单精度浮点数寄存器 rs1 中的值转换为整数，并将结果存储到整数寄存器 rd 中。
class FloatToIntInstruction : public AsmInstruction {
  RegisterPtr src_;
  RegisterPtr target_;
public:
  FloatToIntInstruction(RegisterPtr &src, RegisterPtr &target) : AsmInstruction(AsmInstructionType::FTOI) {
    src_ = src;
    target_ = target;
  }

  auto ToString() -> std::string override;
};

// fmv.w.x f0, a0 将整数a0的值移动到浮点寄存器f0
class MoveIntToFloatInstruction : public AsmInstruction {
  RegisterPtr target_;
  RegisterPtr src_;
public:
  MoveIntToFloatInstruction(RegisterPtr &target, RegisterPtr &src) : AsmInstruction(AsmInstructionType::MITOF) {
    target_ = target;
    src_ = src;
  }

  auto ToString() -> std::string override;

};

class XorInstruction : public AsmInstruction {
  RegisterPtr target_;
  RegisterPtr src1_;
  RegisterPtr src2_;
public:
  XorInstruction(RegisterPtr &target, RegisterPtr &src1, RegisterPtr &src2) : AsmInstruction(AsmInstructionType::XOR) {
    target_ = target;
    src1_ = src1;
    src2_ = src2;
  }

  auto ToString() -> std::string override;
};

/**
 * SLTU rd, x0, rs2 （x0 是 0 号寄存器）若 rs2 不等于 0 把 rd 置 1，否则置 0（汇编伪指令 SNEZ rd, rs ）
 */
class SNEZInstruction : public AsmInstruction {
  RegisterPtr rd_;
  RegisterPtr rs_;
public:
  SNEZInstruction(RegisterPtr &rd, RegisterPtr &rs) : AsmInstruction(AsmInstructionType::SLTU) {
    rd_ = rd;
    rs_ = rs;
  }

  auto ToString() -> std::string override;
};

class XoriInstruction : public AsmInstruction {
  RegisterPtr rd_;
  RegisterPtr rs_;
  Imm12Ptr imm_;
public:
  XoriInstruction(RegisterPtr &rd, RegisterPtr &rs, Imm12Ptr &imm) : AsmInstruction(AsmInstructionType::XORI) {
    rd_ = rd;
    rs_ = rs;
    imm_ = imm;
  }

  auto ToString() -> std::string override;
};

// 需要区分int和float
auto CreateBinInstruction(const std::shared_ptr<IRStatement> &ir,
                          const std::vector<RegisterPtr> &regs) -> std::shared_ptr<BinaryInstruction>;

} // namespace SCNUCC



#endif //SCNUCC_ASMINSTRUCTION_H
