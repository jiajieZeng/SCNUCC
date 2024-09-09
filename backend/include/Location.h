//
// 
// 定义Location，包含stack类，register类，argstack类，Heap类表示一个变量在栈上还是寄存器、全局区上

#ifndef SCNUCC_LOCATION_H
#define SCNUCC_LOCATION_H

#include <string>
#include <memory>
#include "./riscv.h"

namespace SCNUCC {

class Location {
public:
  friend class RegisterManager;
  friend class RegisterAllocater;
  enum LocationType {
    REGISTER,
    STACK,
    ArgsSTACK,
    HEAP,
    IMMEDIATE12,
    IMMEDIATE20,
    IMMEDIATE32,
    LABEL,
  };
  LocationType type;

  std::string name_;

  auto ToString() const -> std::string {
    return name_;
  }

  // 重载==运算符
  virtual auto operator==(const Location &loc) const -> bool {
    return this->ToString() == loc.ToString();
  }

protected:
  explicit Location(LocationType type) : type(type) {}
};

class Stack;

class Register : public Location {
public:
  friend class RegisterManager;
  friend class RegisterAllocater;
  RegisterName reg_;
  std::shared_ptr<Stack> new_data_home_;
  std::shared_ptr<Stack> old_data_stk_;
  Register(const Register &reg) = delete;
  bool no_need_to_load_store = false;
protected:
  explicit Register(RegisterName reg);
};

class Stack : public Location {
public:
  static size_t stack_size_;
  friend class RegisterManager;
  friend class RegisterAllocater;
  int offset_;
  size_t size_ = 4;
  RegisterName base_ = RegisterName::SP;
  std::shared_ptr<Register> back_to_;
  std::shared_ptr<Register> save_to_;
  Stack(const Stack &stack) = delete;
protected:
  explicit Stack();
  explicit Stack(size_t size);
  explicit Stack(RegisterName base, int offset);
};

class ArgsStack: public Location {
public:
  friend class RegisterManager;
  friend class RegisterAllocater;
  int offset_;
  ArgsStack(const ArgsStack &args_stack) = delete;
protected:
  explicit ArgsStack(int offset);
};

class Heap: public Location {
public:
  int offset_;
  std::string label_;  // 数据段标记
  Heap(int offset, std::string &label) : Location(LocationType::HEAP) {
    offset_ = offset;
    label_ = label;
    name_ = label_ + "+" + std::to_string(offset);
  }
};

//class ArrayLoc: public Location {
//public:
//  friend class RegisterManager;
//  friend class RegisterAllocater;
//  int offset_;
//  RegisterName reg_;
//  ArrayLoc(const ArrayLoc &) = delete;
//  explicit ArrayLoc(RegisterName reg, int offset);
//};

class Immediate12 : public Location {
  friend class RegisterManager;
  friend class RegisterAllocater;
  int imm_;  // 用十六进制表示的立即数，包括整数和浮点数
  float float_val_;
public:
  bool is_int_;
  auto GetVal() -> std::string;
  Immediate12(const Immediate12 &imm) = delete;
  Immediate12(int imm, bool is_int);
  Immediate12(float imm);
};

class Immediate20 : public Location {
  friend class RegisterManager;
  friend class RegisterAllocater;
  int imm_;  // 用十六进制表示的立即数，包括整数和浮点数
  float float_val_;
public:
  bool is_int_;
  auto GetVal() -> std::string;
  Immediate20(const Immediate20 &imm) = delete;
  Immediate20(int imm, bool is_int);
  Immediate20(float imm);
};

class Immediate32 : public Location {
  friend class RegisterManager;
  friend class RegisterAllocater;
  int imm_;  // 用十六进制表示的立即数，包括整数和浮点数
  float float_val_;
public:
  bool is_int_;
  auto GetVal() -> std::string;
  Immediate32(const Immediate32 &imm) = delete;
  Immediate32(int imm, bool is_int);
  Immediate32(float imm);
};

}
#endif //SCNUCC_LOCATION_H
