#include <iostream>
#include <sstream>
#include <iomanip>
#include "../include/Location.h"
#include "../include/riscv.h"

namespace SCNUCC {

Register::Register(RegisterName reg) : Location(LocationType::REGISTER), reg_(reg) {
  name_ = reg2str[reg];
}

size_t Stack::stack_size_ = 16; // 留给s0和ra寄存器的栈空间

Stack::Stack() : Location(LocationType::STACK) {
  int i_stack_size = (int)stack_size_;
  offset_ = - stack_size_ - 8;
  stack_size_ += 8;
  name_ = std::to_string(offset_) + "(s0)";
}

Stack::Stack(size_t size) : Location(LocationType::STACK) {
  int i_stack_size = (int)stack_size_;
  offset_ = - i_stack_size - (int)size;
  stack_size_ += size;
  size_ = size;
  name_ = std::to_string(offset_) + "(s0)";
}

Stack::Stack(RegisterName base, int offset) : Location(LocationType::STACK){
  offset_ = offset;
  base_ = base;
  name_ = std::to_string(offset_) + "(" + reg2str[base_] + ")";
}

ArgsStack::ArgsStack(int offset) : Location(LocationType::ArgsSTACK) {
  offset_ = offset;
  name_ = std::to_string(offset_) + "(sp)";
}

//ArrayLoc::ArrayLoc(RegisterName reg, int offset) : Location(LocationType::ArrayLoc) {
//  reg_ = reg;
//  offset_ = offset;
//  name_ = std::to_string(offset_) + "(" + reg2str[reg_] + ")";
//}

Immediate12::Immediate12(int imm, bool is_int) : Location(LocationType::IMMEDIATE12), imm_(imm), is_int_(is_int) {
  std::stringstream ss;
  ss << std::hex << imm_;
  name_ = "0x" + ss.str();
}

Immediate12::Immediate12(float imm) : Location(LocationType::IMMEDIATE12), float_val_(imm), is_int_(false) {
  double x = float_val_;
  std::stringstream ss;
  unsigned long long *int_val = reinterpret_cast<unsigned long long*>(&x);
  ss << std::hex << std::setfill('0') << std::setw(16) << *int_val;
  name_ = "0x" + ss.str();
}

auto Immediate12::GetVal() -> std::string {
  // TODO: 限制imm的位数？
  if (is_int_) {
    return std::to_string(imm_);
  } else {
    return name_;
  }
}

Immediate20::Immediate20(int imm, bool is_int) : Location(LocationType::IMMEDIATE20), imm_(imm), is_int_(is_int) {
  std::stringstream ss;
  ss << std::hex << imm_;
  name_ = "0x" + ss.str();
}

Immediate20::Immediate20(float imm) : Location(LocationType::IMMEDIATE20), float_val_(imm), is_int_(false) {
  double x = float_val_;
  std::stringstream ss;
  unsigned long long *int_val = reinterpret_cast<unsigned long long*>(&x);
  ss << std::hex << std::setfill('0') << std::setw(16) << *int_val;
  name_ = "0x" + ss.str();
}

auto Immediate20::GetVal() -> std::string {
  if (is_int_) {
    return std::to_string(imm_);
  } else {
    return name_;
  }
}

Immediate32::Immediate32(int imm, bool is_int) : Location(LocationType::IMMEDIATE32), imm_(imm), is_int_(is_int) {
  std::stringstream ss;
  ss << std::hex << imm_;
  name_ = "0x" + ss.str();
}

Immediate32::Immediate32(float imm) : Location(LocationType::IMMEDIATE32), float_val_(imm), is_int_(false) {
  double x = float_val_;
  std::stringstream ss;
  unsigned long long *int_val = reinterpret_cast<unsigned long long*>(&x);
  ss << std::hex << std::setfill('0') << std::setw(16) << *int_val;
  name_ = "0x" + ss.str();
}

auto Immediate32::GetVal() -> std::string {
  if (is_int_) {
    return std::to_string(imm_);
  } else {
    return name_;
  }
}

}