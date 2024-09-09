//
// Created by 邓实诚 on 2024/7/22.
//

#ifndef SCNUCC_IRVARIABLE_H
#define SCNUCC_IRVARIABLE_H

#include <memory>
#include <string>
#include <vector>
#include <any>

namespace SCNUCC {

class Location;

class IRStatement;

class IRVariable {
public:
  enum class Type {
    INT,
    FLOAT,
    INT_POINTER,
    FLOAT_POINTER,
    LABEL,
    INT_IMM,
    FLOAT_IMM,
    BOOL, // CmpIR返回的结果 通常会被zero_ext变成INT
    BOOL_IMM, // 通常会被zero_ext变成INT_IMM
    VOID,
  };
  Type type_;

  static size_t variable_cnt_;

  std::shared_ptr<IRStatement> defined_in_;
  std::vector<std::shared_ptr<IRVariable>> used_by_;
  std::vector<std::shared_ptr<IRVariable>> uses_;
  std::string name_;

  int int_value_;
  float float_value_;

  std::shared_ptr<Location> home_location_;
  std::shared_ptr<Location> array_location_;

  IRVariable(Type type) : type_(type) { 
    name_ = "", int_value_ = 0, float_value_ = 0;
    defined_in_ = nullptr;
    used_by_.resize(0);
    uses_.resize(0);
    home_location_ = nullptr;
    array_location_ = nullptr;
  }

  auto IsImm() -> bool {
    return type_ == Type::INT_IMM || type_ == Type::FLOAT_IMM || type_ == Type::BOOL_IMM;
  }

  auto IsLabel() -> bool {
    return type_ == Type::LABEL;
  };

  auto IsPointer() -> bool {
    return type_ == Type::INT_POINTER || type_ == Type::FLOAT_POINTER;
  }

  auto IsIntReg() -> bool {
    return type_ == Type::INT || type_ == Type::INT_IMM || type_ == Type::BOOL || type_ == Type::BOOL_IMM || type_ == Type::INT_POINTER || type_ == Type::FLOAT_POINTER;
  }

  auto IsFloatReg() -> bool {
    return type_ == Type::FLOAT || type_ == Type::FLOAT_IMM;
  }

  auto IsGbl() -> bool;

  /**
   * 最后对IR重新编号的时候需要调用这个函数，设置name_
   * 如果是ToSSA，那么就是再Adjust函数调用IR的GetSequenceID()函数里面调用
   * GetSequenceID()函数里面已经调用了这个函数
   */
  auto SetName(std::string name) ->void { name_ = name; }
};
}

#endif //SCNUCC_IRVARIABLE_H
