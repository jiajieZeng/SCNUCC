//
//  .
//
#include <cassert>
#include <utility>
#include <cmath>
#include <algorithm>

#include "../include/Block.h"
#include "../include/IRStatement.h"
#include "../include/SymbolTable.h"
#include "../include/Util.h"


namespace SCNUCC {

size_t IRStatement::def_cnt_ = 0;

IRStatement::IRStatement(IROP::IROP op, IRTYPE::IRTYPE type, size_t def_id) : op_(op), type_(type), def_id_(def_id),
                                                                              sequence_id_(114514) {

}


auto IRStatement::GetOp() const -> IROP::IROP {
  return op_;
}

void IRStatement::SetOp(IROP::IROP op) {
  op_ = op;
}

void IRStatement::AddOprnd(std::shared_ptr<IRStatement> oprnd) {
  // 构建 def-use 链
  oprnds_.push_back(std::move(oprnd));
}

auto IRStatement::LinkDefUse(const std::shared_ptr<IRStatement> &use) -> void {
  uses_.push_back(use);
  var_->uses_.push_back(use->GetResultVar());

  use->users_.push_back(shared_from_this());
  use->GetResultVar()->used_by_.push_back(var_);
}

auto IRStatement::GetID() const -> size_t {
  return def_id_;
}

auto IRStatement::GetIRType() const -> IRTYPE::IRTYPE {
  return type_;
}

auto IRStatement::ToSSA() const -> std::string {
  std::stringstream ssa;
  ssa << "%" << sequence_id_ /*def_id_*/ << " = " << IR_OP_STR[op_] << " " << IR_TYPE_STR[type_];
  for (size_t i = 0; i < oprnds_.size(); i++) {
    ssa << " " << oprnds_[i]->GetName();
    if (i != oprnds_.size() - 1) {
      ssa << ",";
    }
  }
  return ssa.str();
}

auto IRStatement::GetName() const -> std::string {
  // return "%" + std::to_string(def_id_);
  return "%" + std::to_string(sequence_id_);
}

auto IRStatement::GetConstantVal() const -> std::any {
  return 0;
}

auto IRStatement::GetOprndSize() const -> size_t {
  return oprnds_.size();
}

auto IRStatement::SetSequenceID(size_t id) -> void {
  sequence_id_ = id;
  var_->SetName(this->GetName());
}

auto IRStatement::GetSequenceID() const -> size_t {
  return sequence_id_;
}

auto IRStatement::GetOprnds() -> std::vector<std::shared_ptr<IRStatement>> & {
  return oprnds_;
}

auto IRStatement::GetResultVar() -> std::shared_ptr<IRVariable> & {
  return var_;
}

auto IRStatement::InitVariable() -> void {
  throw "Cannot call IRStatement::InitVariable";
}

auto IRStatement::SetDefUse() -> void {
  for (auto &stmt: oprnds_) {
    auto oprand_var = stmt->GetResultVar();
    var_->uses_.push_back(oprand_var);
    oprand_var->used_by_.push_back(var_);
  }
  var_->name_ = this->GetName();
}

auto IRStatement::GetOperands() -> std::vector<std::shared_ptr<IRVariable>> & {
  return var_->uses_;
}

ArithmeticIR::ArithmeticIR(IROP::IROP op, IRTYPE::IRTYPE type) : IRStatement(op, type, IRStatement::def_cnt_++) {
  assert(op == IROP::IR_ADD || op == IROP::IR_SUB || op == IROP::IR_MUL || op == IROP::IR_SDIV || op == IROP::IR_SREM ||
         op == IROP::IR_FADD ||
         op == IROP::IR_FSUB || op == IROP::IR_FMUL || op == IROP::IR_FDIV || op == IROP::IR_FREM);

}

auto ArithmeticIR::InitVariable() -> void {
  // std::cout << "Call ArithemticIR::InitVariable" << std::endl;
  if (op_ == IROP::IR_ADD || op_ == IROP::IR_SUB || op_ == IROP::IR_MUL || op_ == IROP::IR_SDIV ||
      op_ == IROP::IR_SREM) {
    var_ = std::make_shared<IRVariable>(IRVariable::Type::INT);
  } else {
    var_ = std::make_shared<IRVariable>(IRVariable::Type::FLOAT);
  }
  // std::cout << "Call ArithemeticIR::InitVariable" << std::endl;
  var_->defined_in_ = shared_from_this();
}

auto ArithmeticIR::ToSSA() const -> std::string {
  std::stringstream ssa;
  ssa << "%" << sequence_id_ /*def_id_*/ << " = " << IR_OP_STR[op_] << " " << IR_TYPE_STR[type_];
  for (size_t i = 0; i < oprnds_.size(); i++) {
    ssa << " " << oprnds_[i]->GetName();
    if (i != oprnds_.size() - 1) {
      ssa << ",";
    }
  }
  return ssa.str();
}

CmpIR::CmpIR(IROP::IROP op, IRTYPE::IRTYPE type) : IRStatement(op, type, IRStatement::def_cnt_++) {
  assert(op == IROP::IR_IGT || op == IROP::IR_IGE || op == IROP::IR_ILT || op == IROP::IR_ILE || op == IROP::IR_IEQ ||
         op == IROP::IR_INE ||
         op == IROP::IR_FGT || op == IROP::IR_FGE || op == IROP::IR_FLT || op == IROP::IR_FLE || op == IROP::IR_FEQ ||
         op == IROP::IR_FNE);
}

auto CmpIR::InitVariable() -> void {
  // std::cout << "Call CmpIR::InitVariable" << std::endl;
  var_ = std::make_shared<IRVariable>(IRVariable::Type::BOOL);
  var_->defined_in_ = shared_from_this();
}

BRIR::BRIR(IRTYPE::IRTYPE type, std::shared_ptr<Block> block) : IRStatement(IROP::IR_BR, type, INT_MAX),
                                                                block_(std::move(block)) {

}

auto BRIR::InitVariable() -> void {
  // std::cout << "Call BRIR::InitVariable" << std::endl;
  // TODO: 真的是VOID吗？
  var_ = std::make_shared<IRVariable>(IRVariable::Type::VOID);
  var_->defined_in_ = shared_from_this();
}

auto BRIR::AddBranch(const std::shared_ptr<Block> &block) -> void {
  // block语句的第一条语句是block声明语句
  auto stmt = block->GetStmts()[0];
  oprnds_.push_back(stmt);
}

auto BRIR::AddTrueBranch(const std::shared_ptr<Block> &block) -> void {
  if (oprnds_.size() == 1) {  // 只有一个条件操作数
    auto stmt = block->GetStmts()[0];
    oprnds_.push_back(stmt);
  } else if (oprnds_.size() == 2) {
    throw "True Branch already added";
  } else {
    auto stmt = block->GetStmts()[0];
    oprnds_[1] = stmt;
  }
}

auto BRIR::AddFalseBranch(const std::shared_ptr<Block> &block) -> void {
  if (oprnds_.size() == 1) {
    oprnds_.push_back(nullptr);
    auto stmt = block->GetStmts()[0];
    oprnds_.push_back(stmt);
  } else if (oprnds_.size() == 2) {
    auto stmt = block->GetStmts()[0];
    oprnds_.push_back(stmt);
  } else {
    throw "False Branch already added";
  }
}

auto BRIR::GetTrueBranch() const -> std::shared_ptr<IRStatement> {
  if (oprnds_.size() == 1) {
    return oprnds_[0];
  }
  return oprnds_[1];
}

auto BRIR::GetFalseBranch() const -> std::shared_ptr<IRStatement> {
  return oprnds_[2];
}

auto BRIR::GetBlock() const -> std::shared_ptr<Block> {
  return block_;
}

auto BRIR::SetBlock(const std::shared_ptr<Block> &block) -> void {
  block_ = block;
}

auto BRIR::ToSSA() const -> std::string {
  std::stringstream ssa;
  if (this->oprnds_.size() == 1) {
    assert(this->oprnds_[0]->GetOp() == IROP::IR_BLOCKDECL);
    ssa << "br " << "label " << oprnds_[0]->GetName();
    return ssa.str();
  }
  ssa << "br " << IR_TYPE_STR[type_] << " " << oprnds_[0]->GetName() << ", label " << oprnds_[1]->GetName()
      << ", label " << oprnds_[2]->GetName();

  return ssa.str();
}

auto BRIR::GetCond() const -> std::shared_ptr<IRStatement> {
  return oprnds_[0];
}

RetIR::RetIR(IRTYPE::IRTYPE type) : IRStatement(IROP::IR_RET, type, INT_MAX) {
  null_var_ = nullptr;
}

auto RetIR::InitVariable() -> void {
  // std::cout << "Call RetIR::InitVariable" << std::endl;
  // TODO: 有返回值的时候可能要在外面加上值？
  var_ = std::make_shared<IRVariable>(IRVariable::Type::VOID);
  var_->defined_in_ = shared_from_this();
}

auto RetIR::GetResultVar() -> std::shared_ptr<IRVariable> & {
  return null_var_;
}

auto RetIR::ToSSA() const -> std::string {
  std::stringstream ssa;
  if (this->oprnds_.size() == 0) {
    ssa << "ret void";
    return ssa.str();
  }
  ssa << "ret " << IR_TYPE_STR[this->type_] << " " << oprnds_[0]->GetName();
  return ssa.str();
}

AllocaIR::AllocaIR(IRTYPE::IRTYPE type) : IRStatement(IROP::IR_ALLOCA, type, IRStatement::def_cnt_++) {
}

auto AllocaIR::InitVariable() -> void {
  throw "Use AllocaIR";
}

auto AllocaIR::ToSSA() const -> std::string {
  std::stringstream ssa;
  ssa << "%" << sequence_id_ /*def_id_*/ << " = " << IR_OP_STR[op_] << " " << IR_TYPE_STR[type_];
  return ssa.str();
}

LoadIR::LoadIR(IRTYPE::IRTYPE type) : IRStatement(IROP::IR_LRD, type, IRStatement::def_cnt_++) {

}

auto LoadIR::InitVariable() -> void {
  // std::cout << "Call LordIR::InitVariable" << std::endl;
  IRVariable::Type ret_type;
  if (type_ == IRTYPE::IR_I32) {
    ret_type = IRVariable::Type::INT;
  } else if (type_ == IRTYPE::IR_FLOAT) {
    ret_type = IRVariable::Type::FLOAT;
  } else if (type_ == IRTYPE::IR_INT_PTR) {
    ret_type = IRVariable::Type::INT_POINTER;
  } else if (type_ == IRTYPE::IR_INT_PTR) {
    ret_type = IRVariable::Type::FLOAT_POINTER;
  }
  var_ = std::make_shared<IRVariable>(ret_type);
  var_->defined_in_ = shared_from_this();
}

auto LoadIR::ToSSA() const -> std::string {
  std::stringstream ssa;
  ssa << "%" << sequence_id_ /*def_id_*/ << " = " << IR_OP_STR[op_] << " " << IR_TYPE_STR[type_] << ", ";
  ssa << IR_TYPE_STR[type_] << "* " << oprnds_[0]->GetName();
  return ssa.str();
}

StoreIR::StoreIR(IRTYPE::IRTYPE type) : IRStatement(IROP::IR_STR, type, INT_MAX) {

}

auto StoreIR::InitVariable() -> void {
  // std::cout << "Call StoreIR::InitVariable" << std::endl;
  IRVariable::Type ret_type;
  // if (type_ == IRTYPE::IR_I32) {
  //   ret_type = IRVariable::Type::INT;
  // } else if (type_ == IRTYPE::IR_FLOAT) {
  //   ret_type = IRVariable::Type::FLOAT;
  // } else if (type_ == IRTYPE::IR_INT_PTR) {
  //   ret_type = IRVariable::Type::INT_POINTER;
  // } else if (type_ == IRTYPE::IR_INT_PTR) {
  //   ret_type = IRVariable::Type::FLOAT_POINTER;
  // }
  ret_type = IRVariable::Type::VOID;
  var_ = std::make_shared<IRVariable>(ret_type);
  var_->defined_in_ = shared_from_this(); 
}

auto StoreIR::ToSSA() const -> std::string {
  std::stringstream ssa;
  ssa << "store " << IR_TYPE_STR[type_] << " " << oprnds_[0]->GetName() << ", " << IR_TYPE_STR[type_] << "* "
      << oprnds_[1]->GetName();
  return ssa.str();
}

PhiIR::PhiIR(IRTYPE::IRTYPE type) : IRStatement(IROP::IR_PHI, type, IRStatement::def_cnt_++) {

}

auto PhiIR::InitVariable() -> void {
  // std::cout << "Call PhiIR::InitVariable" << std::endl;
  IRVariable::Type ret_type;
  if (type_ == IRTYPE::IR_I32) {
    ret_type = IRVariable::Type::INT;
  } else if (type_ == IRTYPE::IR_FLOAT) {
    ret_type = IRVariable::Type::FLOAT;
  } else if (type_ == IRTYPE::IR_INT_PTR) {
    ret_type = IRVariable::Type::INT_POINTER;
  } else if (type_ == IRTYPE::IR_FLOAT_PTR) {
    ret_type = IRVariable::Type::FLOAT_POINTER;
  }
  var_ = std::make_shared<IRVariable>(ret_type);
  var_->defined_in_ = shared_from_this();
}

auto PhiIR::ToSSA() const -> std::string {
  std::stringstream ssa;
  // ssa << "%" << def_id_ << " = " << IR_OP_STR[op_] << " " << IR_TYPE_STR[type_];
  ssa << "%" << sequence_id_ << " = " << IR_OP_STR[op_] << " " << IR_TYPE_STR[type_];
  for (size_t i = 0; i < oprnds_.size(); i++) {
    ssa << " [" << "" << oprnds_[i]->GetName() << ", " << oprnds_[i + 1]->GetName() << "]";
    i++;
    if (i != oprnds_.size() - 1) {
      ssa << ",";
    }
  }
  return ssa.str();
}

auto PhiIR::AddUser(std::shared_ptr<PhiIR> user) -> void {
  users_.push_back(std::move(user));
}

auto PhiIR::GetUsers() -> std::vector<std::shared_ptr<PhiIR>> {
  return users_;
}

auto PhiIR::SetIRType(IRTYPE::IRTYPE type) -> void {
  type_ = type;
  if (type_ == IRTYPE::IR_I32) {
    var_->type_ = IRVariable::Type::INT;
  } else if (type_ == IRTYPE::IR_FLOAT) {
    var_->type_ = IRVariable::Type::FLOAT;
  } else if (type_ == IRTYPE::IR_INT_PTR) {
    var_->type_ = IRVariable::Type::INT_POINTER;
  } else if (type_ == IRTYPE::IR_FLOAT_PTR) {
    var_->type_ = IRVariable::Type::FLOAT_POINTER;
  }
}

CallIR::CallIR(IRTYPE::IRTYPE type, std::string func_name, std::vector<Type> params_type)
        : func_name_(func_name), params_type_(std::move(params_type)),
          IRStatement(IROP::IR_CALL, type, IRStatement::def_cnt_++) {

}

auto CallIR::InitVariable() -> void {
  // std::cout << "Call CallIR::InitVariable" << std::endl;
  IRVariable::Type ret_type;
  if (type_ == IRTYPE::IR_VOID) {
    ret_type = IRVariable::Type::VOID;
  } else if (type_ == IRTYPE::IR_I32) {
    ret_type = IRVariable::Type::INT;
  } else {
    ret_type = IRVariable::Type::FLOAT;
  }
  var_ = std::make_shared<IRVariable>(ret_type);
  var_->defined_in_ = shared_from_this();
}

auto CallIR::GetParamsType() const -> std::vector<Type> {
  return params_type_;
}

auto CallIR::GetFuncName() const -> std::string {
  return func_name_;
}

auto CallIR::ToSSA() const -> std::string {
  std::stringstream ssa;

  if (type_ == IRTYPE::IR_VOID) {
    // 特判 putf函数
    if (params_type_.size() && params_type_[0].base_type_ == BaseType::STRING) {
      ssa << "call void (ptr, ...) @putf";
    } else {
      ssa << "call void @" << func_name_;
    }

  } else {
    ssa << GetName() << " = call noundef " << IR_TYPE_STR[type_] << " @" << func_name_;
  }
  // for循环输出param type
  // for (size_t i = 0; i < params_type_.size(); i++) {
  //   if (i) {
  //     ssa << ", ";
  //   }
  //   std::string str_type = "";
  //   if (params_type_[i] == IRTYPE::IR_I32) {
  //     str_type = "int";
  //   } else if (params_type_[i] == IRTYPE::IR_FLOAT) {
  //     str_type = "float";
  //   } else if (params_type_[i] == IRTYPE::IR_INT_PTR) {
  //     str_type = "int*";
  //   } else if (params_type_[i] == IRTYPE::IR_FLOAT_PTR) {
  //     str_type = "float*";
  //   }
  //   ssa << str_type;
  // }
  // ssa << ")";
  ssa << "(";
  if (params_type_.size()) {
    // for循环输出参数
    for (size_t i = 0; i < params_type_.size(); i++) {
      if (i) {
        ssa << ", ";
      }
      // putf的字符串
      if (params_type_[i].base_type_ == BaseType::STRING) {
        // 字符串池的第几个
        ssa << "ptr noundef @.str.";
        ssa << Util::Callid2Poolid[Util::CALLIR_STRING_ID++];
        continue;
      } else if (params_type_[i].base_type_ == BaseType::INT) {
        if (params_type_[i].array_shape_.size()) {
          ssa << "i32*";
        } else {
          ssa << "i32";
        }
      } else if (params_type_[i].base_type_ == BaseType::FLOAT && func_name_ == "putf") {
        ssa << "double";
      } else {
        if (params_type_[i].array_shape_.size()) {
          ssa << "float*";
        } else {
          ssa << "float";
        }
      }

      if (func_name_ == "putf") {
        if (params_type_[i].base_type_ == BaseType::FLOAT) {
          ssa << " noundef ";
          std::string hex_str = oprnds_[i - 1]->GetName().substr(2);

          uint64_t hex_value = std::stoull(hex_str, nullptr, 16);
          double double_value;
          std::memcpy(&double_value, &hex_value, sizeof(double));

          ssa << std::scientific << double_value;
        } else {
          ssa << " noundef " << oprnds_[i - 1]->GetName();
        }
      } else {
        ssa << " noundef " << oprnds_[i]->GetName();
      }


    }
  }
  ssa << ")";
  return ssa.str();
}

IntToFloatIR::IntToFloatIR() : IRStatement(IROP::IR_ITOF, IRTYPE::IR_I32, IRStatement::def_cnt_++) {
}

auto IntToFloatIR::InitVariable() -> void {
  // std::cout << "Call IntToFLoatIR::InitVariable" << std::endl;
  var_ = std::make_shared<IRVariable>(IRVariable::Type::FLOAT);
  var_->defined_in_ = shared_from_this();
}

auto IntToFloatIR::ToSSA() const -> std::string {
  std::stringstream ssa;
  ssa << "%" << sequence_id_ /*def_id_*/ << " = " << IR_OP_STR[op_] << " " << IR_TYPE_STR[type_] << " "
      << oprnds_[0]->GetName()
      << " to " << IR_TYPE_STR[IRTYPE::IR_FLOAT];
  return ssa.str();
}

FloatToIntIR::FloatToIntIR() : IRStatement(IROP::IR_FTOI, IRTYPE::IR_FLOAT, IRStatement::def_cnt_++) {

}

auto FloatToIntIR::InitVariable() -> void {
  // std::cout << "Call FLoatToIntIR::InitVariable" << std::endl;
  var_ = std::make_shared<IRVariable>(IRVariable::Type::INT);
  var_->defined_in_ = shared_from_this();
}

auto FloatToIntIR::ToSSA() const -> std::string {
  std::stringstream ssa;
  ssa << "%" << sequence_id_ /*def_id_*/ << " = " << IR_OP_STR[op_] << " " << IR_TYPE_STR[type_] << " "
      << oprnds_[0]->GetName()
      << " to " << IR_TYPE_STR[IRTYPE::IR_I32];
  return ssa.str();
}

BoolToIntIR::BoolToIntIR() : IRStatement(IROP::IR_BTOI, IRTYPE::IR_I1, IRStatement::def_cnt_++) {

}

auto BoolToIntIR::InitVariable() -> void {
  var_ = std::make_shared<IRVariable>(IRVariable::Type::INT);
  var_->defined_in_ = shared_from_this();
}

auto BoolToIntIR::ToSSA() const -> std::string {
  std::stringstream ssa;
  ssa << "%" << sequence_id_ /*def_id_*/ << " = " << IR_OP_STR[op_] << " " << IR_TYPE_STR[type_] << " "
      << oprnds_[0]->GetName()
      << "to " << IR_TYPE_STR[IRTYPE::IR_I32];
  return ssa.str();
}

BlockDeclIR::BlockDeclIR(size_t block_id) : IRStatement(IROP::IR_BLOCKDECL, IRTYPE::IR_PLACEHOLDER, INT_MAX),
                                            block_id_(block_id) {

}

auto BlockDeclIR::InitVariable() -> void {
  // std::cout << "Call BlockDeclIR::InitVariable" << std::endl;
  var_ = std::make_shared<IRVariable>(IRVariable::Type::LABEL);
  var_->defined_in_ = shared_from_this();
}

auto BlockDeclIR::ToSSA() const -> std::string {
  std::stringstream ssa;
  // ssa << "block" << block_id_ << ":";
  ssa << "block" << sequence_id_ << ":";
  return ssa.str();
}

auto BlockDeclIR::GetID() const -> size_t {
  return block_id_;
}

auto BlockDeclIR::GetName() const -> std::string {
  // return "%block" + std::to_string(block_id_);
  return "%block" + std::to_string(sequence_id_);
}

GblVarDeclIR::GblVarDeclIR(IRTYPE::IRTYPE type, std::shared_ptr<VarSymbol> symbol) : IRStatement(
        IROP::IR_PLACEHOLDER, type, INT_MAX), symbol_(std::move(symbol)) {
}

auto GblVarDeclIR::InitVariable() -> void {
  // std::cout << "Call GblVarDecl::InitVariable" << std::endl;
  if (symbol_->IsPtr()) {
    if (symbol_->GetType().base_type_ == BaseType::INT) {
      var_ = std::make_shared<IRVariable>(IRVariable::Type::INT_POINTER);
    } else {
      var_ = std::make_shared<IRVariable>(IRVariable::Type::FLOAT_POINTER);
    }
  } else {
    if (symbol_->IsConst()) {
      if (symbol_->GetType().base_type_ == BaseType::INT) {
        var_ = std::make_shared<IRVariable>(IRVariable::Type::INT_IMM);
        var_->int_value_ = symbol_->GetScalerValue<int>();
      } else {
        var_ = std::make_shared<IRVariable>(IRVariable::Type::FLOAT_IMM);
        var_->float_value_ = symbol_->GetScalerValue<float>();
      }
    } else {
      if (symbol_->GetType().base_type_ == BaseType::INT) {
        var_ = std::make_shared<IRVariable>(IRVariable::Type::INT);
        var_->int_value_ = symbol_->GetScalerValue<int>();
      } else {
        var_ = std::make_shared<IRVariable>(IRVariable::Type::FLOAT);
        var_->float_value_ = symbol_->GetScalerValue<float>();
      }
    }
  }
  var_->defined_in_ = shared_from_this();
}

auto GblVarDeclIR::SetSuffix(const std::string &suffix) -> void {
  suffix_ = suffix;
}

auto GblVarDeclIR::SetValues(const std::any &values) -> void {
  values_ = values;
}

auto GblVarDeclIR::ToSSA() const -> std::string {
  std::stringstream ssa;
  Type type = symbol_->GetType();
  if (symbol_->IsPtr()) {
    // 数组
    std::vector<size_t> vt = type.array_shape_;
    ssa << "@" << symbol_->GetString() << " = dso_local global ";
    // for (size_t i = 0; i < vt.size(); i++) {
    //   ssa << "[" << vt[i] << " x";
    // }
    // if (type.base_type_ == BaseType::INT) {
    //   ssa << " i32";
    // } else {
    //   ssa << " float";
    // }
    // for (size_t i = 0; i < vt.size(); i++) {
    //   ssa << "]";
    // }
    ssa << " " << suffix_;
  } else {  // 变量
    ssa << "@" << symbol_->GetString() << " = dso_local global " << IR_TYPE_STR[type_] << " ";
    if (type.base_type_ == BaseType::INT) {
      ssa << symbol_->GetScalerValue<int>();
    } else {
      double x = (double) symbol_->GetScalerValue<float>();
      ssa << Util::GetIEEE754(x);
    }
  }
  return ssa.str();
}

auto GblVarDeclIR::IsArray() -> int {
  return symbol_->IsPtr();
}

auto GblVarDeclIR::GetSym() const -> std::shared_ptr<VarSymbol> {
  return symbol_;
}

auto GblVarDeclIR::AllZero() -> int {
  std::ostringstream suffix;
  // std::string suffix = "zeroinitializer";
  auto type = symbol_->GetType();
  std::vector<size_t> vt = type.array_shape_;
  for (size_t i = 0; i < vt.size(); i++) {
    suffix << "[" << vt[i] << " x";
  }
  if (type.base_type_ == BaseType::INT) {
    suffix << " i32";
  } else {
    suffix << " float";
  }
  for (size_t i = 0; i < vt.size(); i++) {
    suffix << "]";
  }
  suffix << " zeroinitializer";
  return suffix.str() == suffix_;
}

ArrayDeclIR::ArrayDeclIR(IRTYPE::IRTYPE type, std::shared_ptr<VarSymbol> symbol)
        : IRStatement(IROP::IR_ALLOCA, type, def_cnt_++), symbol_(std::move(symbol)) {
  array_size_ = 0;
}

auto ArrayDeclIR::InitVariable() -> void {
  // std::cout << "Call ArrayDeclIR::InitVariable" << std::endl;
  if (symbol_->GetType().base_type_ == BaseType::INT) {
    var_ = std::make_shared<IRVariable>(IRVariable::Type::INT_POINTER);
  } else {
    var_ = std::make_shared<IRVariable>(IRVariable::Type::FLOAT_POINTER);
  }
  var_->defined_in_ = shared_from_this();
  var_->name_ = symbol_->GetString();
}

auto ArrayDeclIR::ToSSA() const -> std::string {
  std::stringstream ssa;
  Type type = symbol_->GetType();
  std::vector<size_t> vt = type.array_shape_;
  ssa << GetName() << " = alloca ";
  for (size_t i = 0; i < vt.size(); i++) {
    ssa << "[" << vt[i] << " x ";
  }
  if (type.base_type_ == BaseType::INT) {
    ssa << "i32";
  } else {
    ssa << "float";
  }

  for (size_t i = 0; i < vt.size(); i++) {
    if (i == vt.size() - 1) ssa << "], align 16";
    else ssa << "] ";
  }
  return ssa.str();
}


auto ArrayDeclIR::GetSymbol() const -> std::shared_ptr<VarSymbol> {
  return symbol_;
}

auto ArrayDeclIR::GetArraySize() -> size_t {
  if (array_size_ == 0) {
    size_t size = 4;
    auto shape = symbol_->GetType().array_shape_;
    for (size_t i = 0; i < shape.size(); i++) {
      size *= shape[i];
    }
    array_size_ = size;
  }
  return array_size_;
}

LogicIR::LogicIR(IRTYPE::IRTYPE type) : IRStatement(IROP::IR_XOR, type, IRStatement::def_cnt_++) {

}

auto LogicIR::InitVariable() -> void {
  // std::cout << "Call Logic::InitVariable" << std::endl;
  var_ = std::make_shared<IRVariable>(IRVariable::Type::BOOL);
  var_->defined_in_ = shared_from_this();
}

auto LogicIR::ToSSA() const -> std::string {
  std::stringstream ssa;

  ssa << "%" << sequence_id_ /*def_id_*/ << " = xor " << IR_TYPE_STR[type_] << " " << oprnds_[0]->GetName() << ", true";

  return ssa.str();
}

auto GblVarDeclIR::GetName() const -> std::string {
  return "@" + symbol_->GetString();
}

ConstantIR::ConstantIR(int val, IRTYPE::IRTYPE type) : IRStatement(IROP::IR_PLACEHOLDER, type, -1), int_val_(val),
                                                       float_val_(0) {

}

auto ConstantIR::InitVariable() -> void {
  // std::cout << "Call ConstantIR::InitVariable" << std::endl;
  if (type_ == IRTYPE::IR_CONST_INT || type_ == IRTYPE::IR_CONST_BOOL) {
    var_ = std::make_shared<IRVariable>(IRVariable::Type::INT_IMM);
    var_->int_value_ = int_val_;
  } else {
    var_ = std::make_shared<IRVariable>(IRVariable::Type::FLOAT_IMM);
    var_->float_value_ = float_val_;
  }
  var_->name_ = this->GetName();
  var_->defined_in_ = shared_from_this();
}

ConstantIR::ConstantIR(float val, IRTYPE::IRTYPE type) : IRStatement(IROP::IR_FADD, type, -1), int_val_(0),
                                                         float_val_(val) {
}

std::any ConstantIR::GetConstantVal() const {
  if (type_ == IRTYPE::IR_CONST_FLOAT) {
    return float_val_;
  } else {
    return int_val_;
  }
}

auto ConstantIR::ToSSA() const -> std::string {
  throw "Constant IR cannot called ToSSA";
}

auto ConstantIR::GetName() const -> std::string {
  if (type_ == IRTYPE::IR_CONST_INT || type_ == IRTYPE::IR_CONST_BOOL) {
    return std::to_string(int_val_);
  } else {
    double x = float_val_;
    return Util::GetIEEE754(x);
  }
}

auto ConstantIR::SetIRType(IRTYPE::IRTYPE type) -> void {
  type_ = type;
}

auto ConstantIR::SetVal(int val) -> void {
  int_val_ = val;
}

auto ConstantIR::SetVal(float val) -> void {
  float_val_ = val;
}

FuncParamIR::FuncParamIR(IRTYPE::IRTYPE type, int id) : IRStatement(IROP::IR_PLACEHOLDER, type, def_cnt_++), param_id_(id) {

}

auto FuncParamIR::InitVariable() -> void {
  IRVariable::Type ret_type;
  if (type_ == IRTYPE::IR_I32) {
    ret_type = IRVariable::Type::INT;
  } else if (type_ == IRTYPE::IR_INT_PTR) {
    ret_type = IRVariable::Type::INT_POINTER;
  } else if (type_ == IRTYPE::IR_FLOAT) {
    ret_type = IRVariable::Type::FLOAT;
  } else {
    ret_type = IRVariable::Type::FLOAT_POINTER;
  }
  var_ = std::make_shared<IRVariable>(ret_type);
  var_->defined_in_ = shared_from_this();
}

auto FuncParamIR::ToSSA() const -> std::string {
  throw "FuncParamIR cannot called ToSSA";
}

auto FuncParamIR::GetParamID() -> int {
  return param_id_;
}

BitCastIR::BitCastIR(IRTYPE::IRTYPE type, std::shared_ptr<VarSymbol> symbol)
        : IRStatement(IROP::IR_BITCAST, type, def_cnt_++), symbol_(std::move(symbol)) {
  null_var_ = nullptr;
}

auto BitCastIR::InitVariable() -> void {
  var_ = std::make_shared<IRVariable>(IRVariable::Type::INT_POINTER);
  var_->defined_in_ = shared_from_this();
}

auto BitCastIR::ToSSA() const -> std::string {
  std::stringstream ssa;
  ssa << GetName() << " = bitcast ";
  Type type = symbol_->GetType();
  std::vector<size_t> vt = type.array_shape_;
  long long bytes = 4;
  for (size_t i = 0; i < vt.size(); i++) {
    ssa << "[" << vt[i] << " x ";
    bytes *= (long long) vt[i];
  }
  if (type.base_type_ == BaseType::INT) {
    ssa << "i32";
  } else {
    ssa << "float";
  }
  for (size_t i = 0; i < vt.size(); i++) {
    ssa << "]";
  }
  ssa << "* " << oprnds_[0]->GetName() << " to i8*\n";
  // 再来一个memset
  ssa << "  call void @llvm.memset.p0.i32(i8* align 16 " << GetName() << ", i8 0, i32 " << bytes << ", i1 false)";
  return ssa.str();
}

auto BitCastIR::GetResultVar() -> std::shared_ptr<IRVariable>& {
  return null_var_;
}

GEPIR::GEPIR(IRTYPE::IRTYPE type, std::shared_ptr<VarSymbol> symbol,
             std::vector<std::shared_ptr<IRStatement>> indices)
        : IRStatement(IROP::IR_GEP, type, def_cnt_++), symbol_(std::move(symbol)), indices_(std::move(indices)) {

}

auto GEPIR::InitVariable() -> void {
  if (symbol_->GetType().base_type_ == BaseType::INT) {
    var_ = std::make_shared<IRVariable>(IRVariable::Type::INT_POINTER);
  } else {
    var_ = std::make_shared<IRVariable>(IRVariable::Type::FLOAT_POINTER);
  }
  var_->defined_in_ = shared_from_this();
}

/**
 * GEP有三种情况，第一种是最简单的获取元素GEP
 *  a[0][0][0][0] = 1，这里是访问本地数组和访问全局数组的犯法
 *  %3 = getelementptr inbounds [4 x [2 x [2 x [2 x i32]]]], ptr %2, i32 0, i32 0
 *  %4 = getelementptr inbounds [2 x [2 x [2 x i32]]], ptr %3, i32 0, i32 0
 *  %5 = getelementptr inbounds [2 x [2 x i32]], ptr %4, i32 0, i32 0
 *  %6 = getelementptr inbounds [2 x i32], ptr %5, i32 0, i32 0
 *  store i32 1, ptr %6, align 16
 * 
 * 第二种是：这种感觉还是不能省略，先想想办法
 * foo(a); 这种情况传一个0进来应该就好了
 * 
 * %10 = getelementptr inbounds [4 x [2 x [2 x [2 x i32]]]], ptr %2, i64 0, i64 0
 * call void @foo(int (*) [2][2][2])(ptr noundef %10)
 * 
 * 第三种是：
 * void foo(int a[][2][2][2]) {
 *  a[2][1][1][1] = 0;
 * }
 * define dso_local void @foo(int (*) [2][2][2])(ptr noundef %0) #0 {
 * %2 = alloca ptr, align 8
 * store ptr %0, ptr %2, align 8
 * %3 = load ptr, ptr %2, align 8
 * %4 = getelementptr inbounds [2 x [2 x [2 x i32]]], ptr %3, i64 2
 * %5 = getelementptr inbounds [2 x [2 x [2 x i32]]], ptr %4, i64 0, i64 1
 * %6 = getelementptr inbounds [2 x [2 x i32]], ptr %5, i64 0, i64 1
 * %7 = getelementptr inbounds [2 x i32], ptr %6, i64 0, i64 1
 * store i32 0, ptr %7, align 4
 * ret void
 * }
 *
 * 
*/
auto GEPIR::ToSSA() const -> std::string {
  size_t n = indices_.size();
  std::stringstream ssa;
  ssa << GetName() << " = " << IR_OP_STR[op_] << " ";
  Type type = symbol_->GetType();
  std::vector<size_t> shape = type.array_shape_;
  // 这一位不是0，那么就是本地数组或者直接调用全局数组
  if (shape[0] != 0) {
    for (size_t i = n - 1; i < shape.size(); i++) {
      ssa << "[" << shape[i] << " x ";
    }
    if (type.base_type_ == BaseType::INT) {
      ssa << "i32";
    } else {
      ssa << "float";
    }
    for (size_t i = n - 1; i < shape.size(); i++) {
      ssa << "]";
    }
    ssa << ", ptr " << oprnds_[0]->GetName() << ", i32 0, i32 " << indices_.back()->GetName();
  } else {
    // 此时是通过函数传参数得到的数组, 先明确写清楚再合并代码
    for (size_t i = std::max(static_cast<size_t>(1ul), static_cast<size_t>(n - 1ul)); i < shape.size(); i++) {
      ssa << "[" << shape[i] << " x ";
    }
    if (type.base_type_ == BaseType::INT) {
      ssa << "i32";
    } else {
      ssa << "float";
    }
    for (size_t i = std::max(static_cast<size_t>(1ul), static_cast<size_t>(n - 1ul)); i < shape.size(); i++) {
      ssa << "]";
    }
    ssa << ", ptr " << oprnds_[0]->GetName() << ", ";
    // %4 = getelementptr inbounds [2 x [2 x [2 x i32]]], ptr %3, i64 2
    // 如果indices == 1， gep的是第一维，少一个基地址
    if (indices_.size() > 1) {
      ssa << "i32 0, i32 ";
    } else {
      ssa << "i32 ";
    }
    ssa << indices_.back()->GetName();
  }

  return ssa.str();
}

auto GEPIR::LeftSize() -> int {
  // 这一位不是0，那么就是本地数组或者直接调用全局数组
  int ans = 1;
  size_t n = indices_.size();
  Type type = symbol_->GetType();
  std::vector<size_t> shape = type.array_shape_;
  if (shape[0] != 0) {
    for (size_t i = n; i < shape.size(); i++) {
      ans *= shape[i];
    }
  } else {
    // 此时是通过函数传参数得到的数组, 先明确写清楚再合并代码
    for (size_t i = n; i < shape.size(); i++) {
      ans *= shape[i];
    }
  }
  return ans;
}

auto GEPIR::IsValue() const -> int {
  return (symbol_->GetType().array_shape_.size() == indices_.size());
}


std::unordered_map<IROP::IROP, std::string> IR_OP_STR = {
        {IROP::IR_ADD,     "add"},
        {IROP::IR_SUB,     "sub"},
        {IROP::IR_MUL,     "mul"},
        {IROP::IR_SDIV,    "sdiv"},
        {IROP::IR_SREM,    "srem"},
        {IROP::IR_ITOF,    "sitofp"},
        {IROP::IR_FADD,    "fadd"},
        {IROP::IR_FSUB,    "fsub"},
        {IROP::IR_FMUL,    "fmul"},
        {IROP::IR_FDIV,    "fdiv"},
        {IROP::IR_FREM,    "frem"},
        {IROP::IR_FTOI,    "fptosi"},
        {IROP::IR_IGT,     "icmp sgt"},
        {IROP::IR_IGE,     "icmp sge"},
        {IROP::IR_ILT,     "icmp slt"},
        {IROP::IR_ILE,     "icmp sle"},
        {IROP::IR_IEQ,     "icmp eq"},
        {IROP::IR_INE,     "icmp ne"},
        {IROP::IR_FGT,     "fcmp ogt"},
        {IROP::IR_FGE,     "fcmp oge"},
        {IROP::IR_FLT,     "fcmp olt"},
        {IROP::IR_FLE,     "fcmp ole"},
        {IROP::IR_FEQ,     "fcmp oeq"},
        {IROP::IR_FNE,     "fcmp one"},
        {IROP::IR_NEG,     "neg"},
        {IROP::IR_AND,     "and"},
        {IROP::IR_NOT,     "not"},
        {IROP::IR_OR,      "or"},
        {IROP::IR_ALLOCA,  "alloca"},
        {IROP::IR_STR,     "store"},
        {IROP::IR_LRD,     "load"},
        {IROP::IR_BR,      "br"},
        {IROP::IR_RET,     "ret"},
        {IROP::IR_CALL,    "call"},
        {IROP::IR_PHI,     "phi"},
        {IROP::IR_BTOI,    "zext"},
        {IROP::IR_XOR,     "xor"},
        {IROP::IR_BITCAST, "bitcast"},
        {IROP::IR_GEP,     "getelementptr inbounds"}
}; // IR_OP_STR

std::unordered_map<IRTYPE::IRTYPE, std::string> IR_TYPE_STR = {
        {IRTYPE::IR_I1,          "i1"},
        {IRTYPE::IR_I32,         "i32"},
        {IRTYPE::IR_I64,         "i64"},
        {IRTYPE::IR_FLOAT,       "float"},
        {IRTYPE::IR_DOUBLE,      "double"},
        {IRTYPE::IR_CONST_INT,   "i32"},
        /** bool 常量也用i32来代替了*/
        {IRTYPE::IR_CONST_BOOL,  "i1"},
        {IRTYPE::IR_CONST_FLOAT, "float"},
        {IRTYPE::IR_INT_PTR,     "i32*"},
        {IRTYPE::IR_FLOAT_PTR,   "float*"}
}; // IR_TYPE_STR

} // SCNUCC