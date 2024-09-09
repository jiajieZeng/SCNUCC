//
//  .
//

#ifndef SCNUCC_IRSTATEMENT_H
#define SCNUCC_IRSTATEMENT_H

#include <string>
#include <sstream>
#include <vector>
#include <memory>
#include <any>
#include <climits>
#include "./LLVMIR.h"
#include "./Util.h"
#include "./SymbolTable.h"
#include <cstring>
#include "./IRVariable.h"

namespace SCNUCC {

class Block;

class IRVariable;

class IRStatement : public std::enable_shared_from_this<IRStatement> {
  friend class AsmGenerator;
public:
  /**
   * 构造函数
   * @param op 指令的算子类型，比如add、sub、mul、div等，具体见LLVMIR.h的IROP枚举
   * @param type 指令的操作数类型。LLVM IR大部分指令都需要指定操作数类型，比如i1、i32、float等，具体见LLVMIR.h的IRTYPE枚举
   * @param def_id 定义的ID。比如%3 = add i32 %1, %3，那么ID就是3
   */
  IRStatement(IROP::IROP op, IRTYPE::IRTYPE type, size_t def_id);

  /**
   * 将IR语句转换为SSA形式的字符串
   * @return
   */
  virtual auto ToSSA() const -> std::string;

  /**
   * 获取当前IR语句的操作符。比如%3 = add i32 %1, %3，那么add就是操作符。
   * @return
   */
  virtual auto GetOp() const -> IROP::IROP;

  /**
   * 设置当前IR语句的操作符。比如%3 = add i32 %1, %3，那么add就是操作符。
   * 全部操作符见LLVMIR.h的IROP枚举
   * @param op
   */
  virtual void SetOp(IROP::IROP op);

  /**
   * 添加操作数。比如：
   * %1 = alloca i32 ; stmt1
   * %2 = alloca i32 ; stmt2
   * %3 = add i32 %1, %2 ; stmt3
   * 那么%1和%2就是操作数，stmt1和stmt2就是AddOprnd的参数
   * @param oprnd
   */
  virtual void AddOprnd(std::shared_ptr<IRStatement> oprnd);

  /**
   * 获取当前IR语句的ID。比如%3 = add i32 %1, %3，那么ID就是3
   * @return
   */
  virtual auto GetID() const -> size_t;

  /**
   * 获取当前IR语句的名字。比如%3 = add i32 %1, %3，那么名字就是%3
   * @return
   */
  virtual auto GetName() const -> std::string;

  /**
   * 获取当前IR语句的类型
   * @return IRTYPE::IRTYPE 
  */
  virtual auto GetIRType() const -> IRTYPE::IRTYPE;

  virtual auto GetConstantVal() const -> std::any;

  virtual auto GetOprndSize() const -> size_t;

  virtual auto SetSequenceID(size_t id) -> void;

  virtual auto GetSequenceID() const -> size_t;

  virtual auto GetOprnds() -> std::vector<std::shared_ptr<IRStatement>>&;

  auto LinkDefUse(const std::shared_ptr<IRStatement>& use) -> void;

  /**
   * 定义的ID计数器。每次定义一个新的IR语句，ID就会自增1。
   * br、ret这些statement不会增加ID。
   */
  static size_t def_cnt_;

  virtual auto GetResultVar() -> std::shared_ptr<IRVariable>&;

  virtual auto GetOperands() -> std::vector<std::shared_ptr<IRVariable>>&;

  virtual auto InitVariable() -> void;

  auto SetDefUse() -> void;

  std::vector<std::shared_ptr<IRStatement>> oprnds_; /** 操作数 */

  size_t numbering; // 用于 live analysis

protected:
  friend class AsmGenerator;
  IROP::IROP op_; /** 操作符 */
  size_t def_id_; /** ID，比如%3 = add i32 %1, %3，那么ID就是3 */
  IRTYPE::IRTYPE type_; /** 类型。大部分指令都需要指定操作数的类型，比如i1，i32，float */
  size_t sequence_id_; /** ID, 调整顺序后的id*/
  std::vector<std::shared_ptr<IRStatement>> users_; /** 使用这个IR的IR */
  std::vector<std::shared_ptr<IRStatement>> uses_; /** 这个IR使用的IR */
  std::shared_ptr<IRVariable> var_;
};

class BlockDeclIR;

/**
 * 加减乘除等算术运算的IR语句，比如%3 = add i32 %1, %2
 * 需要注意的是，算子是区分类型，比如add是整数加法，fadd是浮点数加法
 */
class ArithmeticIR : public IRStatement {
public:
  friend class AsmGenerator;
  /**
   * 构造函数
   * @param op 指令的算子类型，比如add、sub、mul、div等，具体见LLVMIR.h的IROP枚举
   * @param type 操作数类型，比如i1、i32、float等，具体见LLVMIR.h的IRTYPE枚举
   */
  ArithmeticIR(IROP::IROP op, IRTYPE::IRTYPE type);

  static std::shared_ptr<ArithmeticIR> Create(IROP::IROP op, IRTYPE::IRTYPE type) {
    auto instance = std::shared_ptr<ArithmeticIR>(new ArithmeticIR(op, type));
    instance->InitVariable();
    return instance;
  }

  auto ToSSA() const -> std::string override;
  
  auto InitVariable() -> void override;
};

/**
 * 比较运算的IR语句，比如%3 = icmp eq i32 %1, %2，%3应该是一个i1类型的变量（即bool值）
 */
class CmpIR : public IRStatement {
public:
  friend class AsmGenerator;
  /**
   * 构造函数
   * @param op 指令的算子类型，比如add、sub、mul、div等，具体见LLVMIR.h的IROP枚举
   * @param type
   */
  CmpIR(IROP::IROP op, IRTYPE::IRTYPE type);

  static std::shared_ptr<CmpIR> Create(IROP::IROP op, IRTYPE::IRTYPE type) {
    auto instance = std::shared_ptr<CmpIR>(new CmpIR(op, type));
    instance->InitVariable();
    return instance;
  }

  auto InitVariable() -> void override;
};

/**
 * 分支语句的IR语句。分支语句有两个类型，无条件分支和有条件分支
 * 无条件分支：br label %block1
 * 有条件分支：br i1 %1, label %block1, label %block2
 * 并且，分支语句的操作数应该是BlockDeclIR类
 */
class BRIR : public IRStatement {
private:
  friend class AsmGenerator;
  std::shared_ptr<Block> block_;  // 这条IR属于哪一个block
public:
  BRIR(IRTYPE::IRTYPE type, std::shared_ptr<Block> block);

  static std::shared_ptr<BRIR> Create(IRTYPE::IRTYPE type, std::shared_ptr<Block> block) {
    auto instance = std::shared_ptr<BRIR>(new BRIR(type, block));
    instance->InitVariable();
    return instance;
  }

  auto ToSSA() const -> std::string override;

  auto AddBranch(const std::shared_ptr<Block> &block) -> void;

  auto AddTrueBranch(const std::shared_ptr<Block> &block) -> void;

  auto GetTrueBranch() const -> std::shared_ptr<IRStatement>;

  auto GetFalseBranch() const -> std::shared_ptr<IRStatement>;

  auto AddFalseBranch(const std::shared_ptr<Block> &block) -> void;

  auto GetBlock() const -> std::shared_ptr<Block>;

  auto SetBlock(const std::shared_ptr<Block> &block) -> void;


  // 获取转移条件
  auto GetCond() const -> std::shared_ptr<IRStatement>;

  auto InitVariable() -> void override;
};

/**
 * 返回语句的IR语句。返回语句有两个类型，无返回值和有返回值
 * 无返回值：ret void
 * 有返回值：ret <type> <value>
 */
class RetIR : public IRStatement {
public:
  RetIR(IRTYPE::IRTYPE type);

  static std::shared_ptr<RetIR> Create(IRTYPE::IRTYPE type) {
    auto instance = std::shared_ptr<RetIR>(new RetIR(type));
    instance->InitVariable();
    return instance;
  }

  auto ToSSA() const -> std::string override;

  auto InitVariable() -> void override;

  auto GetResultVar() -> std::shared_ptr<IRVariable>& override;

private:
  friend class AsmGenerator;
  std::shared_ptr<IRVariable> null_var_;
};

/**
 * alloca语句的IR语句。alloca语句用于在栈上分配内存
 * %3 = alloca i32
 */
class AllocaIR : public IRStatement {
  friend class AsmGenerator;
public:
  AllocaIR(IRTYPE::IRTYPE type);

  static std::shared_ptr<AllocaIR> Create(IRTYPE::IRTYPE type) {
    auto instance = std::shared_ptr<AllocaIR>(new AllocaIR(type));
    instance->InitVariable();
    return instance;
  }

  auto ToSSA() const -> std::string override;

  auto InitVariable() -> void override;
};

/**
 * load语句的IR语句。load语句用于从指针中加载数据
 * %3 = load i32* %2
 */
class LoadIR : public IRStatement {
  friend class AsmGenerator;
public:
  LoadIR(IRTYPE::IRTYPE type);

  static std::shared_ptr<LoadIR> Create(IRTYPE::IRTYPE type) {
    auto instance = std::shared_ptr<LoadIR>(new LoadIR(type));
    instance->InitVariable();
    return instance;
  }

  auto ToSSA() const -> std::string override;

  auto InitVariable() -> void override;
};

/**
 * store语句的IR语句。store语句用于将数据存储到指针中
 * store i32 %1, i32* %2
 */
class StoreIR : public IRStatement {
  friend class AsmGenerator;
public:

  StoreIR(IRTYPE::IRTYPE type);

  static std::shared_ptr<StoreIR> Create(IRTYPE::IRTYPE type) {
    auto instance = std::shared_ptr<StoreIR>(new StoreIR(type));
    instance->InitVariable();
    return instance;
  }

  auto ToSSA() const -> std::string override;

  auto InitVariable() -> void override;
};

/**
 * phi语句的IR语句。phi语句用于处理分支语句的合并
 */
class PhiIR : public IRStatement {
  friend class AsmGenerator;
public:
  PhiIR(IRTYPE::IRTYPE type);

  static std::shared_ptr<PhiIR> Create(IRTYPE::IRTYPE type) {
    auto instance = std::shared_ptr<PhiIR>(new PhiIR(type));
    instance->InitVariable();
    return instance;
  }

  auto ToSSA() const -> std::string override;

  auto AddUser(std::shared_ptr<PhiIR> user) -> void;

  auto GetUsers() -> std::vector<std::shared_ptr<PhiIR>>;

  /**
   * 调整GenPhiForCond的默认的I1，根据操作数的返回值来确定Phi的IRTYPE
  */
  auto SetIRType(IRTYPE::IRTYPE type) -> void;

  auto InitVariable() -> void override;

private:
  friend class AsmGenerator;
  std::vector<std::shared_ptr<PhiIR>> users_;
};

/**
 * call语句的IR语句。call语句用于调用函数
 */
class CallIR : public IRStatement {
public:
  CallIR(IRTYPE::IRTYPE type, std::string func_name, std::vector<Type> params_type);

  static std::shared_ptr<CallIR> Create(IRTYPE::IRTYPE type, std::string func_name, std::vector<Type> params_type) {
    auto instance = std::shared_ptr<CallIR>(new CallIR(type, func_name, params_type));
    instance->InitVariable();
    return instance;
  }

  auto ToSSA() const -> std::string override;

  auto InitVariable() -> void override;

  auto GetFuncName() const -> std::string;

  auto GetParamsType() const -> std::vector<Type>;

private:
  friend class AsmGenerator;
  /**
   * 需要知道
   * 1. 返回类型： 使用父类成员 type_ 表示返回类型
   * 2. 参数数量： 
   * 3. 每一个参数的类型：使用vector存放没个参数的类型，同时知道参数的数量
   * 4. 传入的每一个参数：使用父类成员oprnds_ 存放参数
   * 5. 函数名：使用 string
  */
  std::string func_name_;
  std::vector<Type> params_type_;
};

/**
 * int转float的IR语句
 */
class IntToFloatIR : public IRStatement {
public:
  IntToFloatIR();

  static std::shared_ptr<IntToFloatIR> Create() {
    auto instance = std::shared_ptr<IntToFloatIR>(new IntToFloatIR());
    instance->InitVariable();
    return instance;
  }

  auto ToSSA() const -> std::string override;

  auto InitVariable() -> void override;

};

/**
 * float转int的IR语句
 */
class FloatToIntIR : public IRStatement {
public:
  FloatToIntIR();

  static std::shared_ptr<FloatToIntIR> Create() {
    auto instance = std::shared_ptr<FloatToIntIR>(new FloatToIntIR());
    instance->InitVariable();
    return instance;
  }

  auto ToSSA() const -> std::string override;

  auto InitVariable() -> void override;

};

/**
 * bool转int的IR语句
 */
class BoolToIntIR : public IRStatement {
public:
  BoolToIntIR();

  static std::shared_ptr<BoolToIntIR> Create() {
    auto instance = std::shared_ptr<BoolToIntIR>(new BoolToIntIR());
    instance->InitVariable();
    return instance;
  }

  auto ToSSA() const -> std::string override;

  auto InitVariable() -> void override;
};

/**
 * BlockDeclIR的IR语句。BlockDeclIR用于声明一个Block
 * 比如：
 * block1: ； 这是BlockDeclIR
 *  %1 = alloca i32
 */
class BlockDeclIR : public IRStatement {
public:
  std::shared_ptr<Block> blk_;  // 这条IR是属于哪一个block的

  BlockDeclIR(size_t block_id);

  static std::shared_ptr<BlockDeclIR> Create(size_t block_id) {
    auto instance = std::shared_ptr<BlockDeclIR>(new BlockDeclIR(block_id));
    instance->InitVariable();
    return instance;
  }

  auto ToSSA() const -> std::string override;

  auto GetID() const -> size_t override;

  auto GetName() const -> std::string override;

  auto InitVariable() -> void override;

private:
  friend class AsmGenerator;
  size_t block_id_;
};

/**
 * GblVarDeclIR的IR语句。GblVarDeclIR用于声明一个全局变量。
 * 比如：
 * int a = 1; ；
 * 对应的GblVarDeclIR是： %1 = global i32 1
 */
class GblVarDeclIR : public IRStatement {
public:
  GblVarDeclIR(IRTYPE::IRTYPE type, std::shared_ptr<VarSymbol> symbol);

  static std::shared_ptr<GblVarDeclIR> Create(IRTYPE::IRTYPE type, std::shared_ptr<VarSymbol> symbol) {
    auto instance = std::shared_ptr<GblVarDeclIR>(new GblVarDeclIR(type, symbol));
    instance->InitVariable();
    return instance;
  }

  auto ToSSA() const -> std::string override;

  auto GetName() const -> std::string override;

  auto SetValues(const std::any &values) -> void;

  auto SetSuffix(const std::string &suffix) -> void;

  auto IsArray() -> int;

  auto InitVariable() -> void override;

  auto GetSym() const -> std::shared_ptr<VarSymbol>;

  auto AllZero() -> int;

private:
  friend class AsmGenerator;
  const std::shared_ptr<VarSymbol> symbol_;
  std::any values_;
  std::string suffix_;
};

/**
 * ArrayDeclIR的IR语句。ArrayDeclIR用于声明一个数组。
 * 比如：
 * int a[3] = {1, 2, 3}; // 对应的IR是：%1 = alloca [3 x i32] [i32 1, i32 2, i32 3]
 */
class ArrayDeclIR : public IRStatement {
public:
  // 避免出现作用域的问题，使用symbol
  ArrayDeclIR(IRTYPE::IRTYPE type, std::shared_ptr<VarSymbol> symbol);

  static std::shared_ptr<ArrayDeclIR> Create(IRTYPE::IRTYPE type, std::shared_ptr<VarSymbol> symbol) {
    auto instance = std::shared_ptr<ArrayDeclIR>(new ArrayDeclIR(type, symbol));
    instance->InitVariable();
    return instance;
  }

  auto ToSSA() const -> std::string override;

  auto GetSymbol() const -> std::shared_ptr<VarSymbol>;

  auto InitVariable() -> void override;

  auto GetArraySize() -> size_t;

private:
  friend class AsmGenerator;
  const std::shared_ptr<VarSymbol> symbol_;
  size_t array_size_;
};


/**
 * 逻辑运算的IR语句，目前只有XOR
*/
class LogicIR: public IRStatement {
public:

  LogicIR(IRTYPE::IRTYPE type);

  static std::shared_ptr<LogicIR> Create(IRTYPE::IRTYPE type) {
    auto instance = std::shared_ptr<LogicIR>(new LogicIR(type));
    instance->InitVariable();
    return instance;
  }

  auto ToSSA() const -> std::string override;

  auto InitVariable() -> void override;
};

/**
 * 用来存储数值常量
 * 不可以把这个IR转成SSA，这个IR只能作为别的IR的操作数
*/
class ConstantIR : public IRStatement {
public:
  /**
   * 构造函数
   * @param val  整数常量
   * @param type 操作数类型，比如i1、i32、float等，具体见LLVMIR.h的IRTYPE枚举
   */
  ConstantIR(int val, IRTYPE::IRTYPE type);

  /**
   * 构造函数
   * @param val  浮点数常量
   * @param type 操作数类型，比如i1、i32、float等，具体见LLVMIR.h的IRTYPE枚举
   */
  ConstantIR(float val, IRTYPE::IRTYPE type);

  static std::shared_ptr<ConstantIR> Create(int val, IRTYPE::IRTYPE type) {
    auto instance = std::shared_ptr<ConstantIR>(new ConstantIR(val, type));
    instance->InitVariable();
    return instance;
  }

  static std::shared_ptr<ConstantIR> Create(float val, IRTYPE::IRTYPE type) {
    auto instance = std::shared_ptr<ConstantIR>(new ConstantIR(val, type));
    instance->InitVariable();
    return instance;
  }

  auto GetConstantVal() const -> std::any override;

  /**
   * 返回空字符串
   * @return
   */
  auto ToSSA() const -> std::string override;

  auto GetName() const -> std::string override;

  auto SetIRType(IRTYPE::IRTYPE type) -> void;

  auto SetVal(int val) -> void;

  auto SetVal(float val) -> void;

  auto InitVariable() -> void override;

private:
  friend class AsmGenerator;
  int int_val_; /** 整数常量的数值 */
  float float_val_; /** 浮点数常量的数值 */
};

class FuncParamIR : public IRStatement {
private:
  int param_id_; // 是第几个函数参数

public:
  FuncParamIR(IRTYPE::IRTYPE type, int id);

  static std::shared_ptr<FuncParamIR> Create(IRTYPE::IRTYPE type, int id) {
    auto instance = std::shared_ptr<FuncParamIR>(new FuncParamIR(type, id));
    instance->InitVariable();
    return instance;
  }

  auto ToSSA() const -> std::string override;

  auto InitVariable() -> void override;

  auto GetParamID() -> int;
};

class BitCastIR : public IRStatement {
public:
  BitCastIR(IRTYPE::IRTYPE type, std::shared_ptr<VarSymbol> symbol);

  static std::shared_ptr<BitCastIR> Create(IRTYPE::IRTYPE type, std::shared_ptr<VarSymbol> symbol) {
    auto instance = std::shared_ptr<BitCastIR>(new BitCastIR(type, symbol));
    instance->InitVariable();
    return instance;
  }

  auto ToSSA() const -> std::string override;

  auto InitVariable() -> void override;

  auto GetResultVar() -> std::shared_ptr<IRVariable>& override;
private:
  friend class AsmGenerator;
  const std::shared_ptr<VarSymbol> symbol_;
  std::shared_ptr<IRVariable> null_var_;
};

class GEPIR : public IRStatement {
public:
  GEPIR(IRTYPE::IRTYPE type, std::shared_ptr<VarSymbol> symbol, std::vector<std::shared_ptr<IRStatement>> indices);

  static std::shared_ptr<GEPIR> Create(IRTYPE::IRTYPE type, std::shared_ptr<VarSymbol> symbol, std::vector<std::shared_ptr<IRStatement>> indices) {
    auto instance = std::shared_ptr<GEPIR>(new GEPIR(type, symbol, indices));
    instance->InitVariable();
    return instance;
  }

  auto ToSSA() const -> std::string override;

  auto IsValue() const -> int;

  auto InitVariable() -> void override;

  auto LeftSize() -> int;

private:
  friend class AsmGenerator;
  const std::shared_ptr<VarSymbol> symbol_;
  const std::vector<std::shared_ptr<IRStatement>> indices_;
};

} // SCNUCC

#endif //SCNUCC_IRSTATEMENT_H
