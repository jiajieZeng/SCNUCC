#include <queue>
#include <sstream>
#include "../include/RegisterAllocater.h"
#include "../../frontend/include/ControlFlowGraph.h"
#include "../../frontend/include/IRStatement.h"
#include "../include/Location.h"
#include "../../frontend/include/IRVariable.h"
#include "../include/LiveAnalyzer.h"

namespace SCNUCC {
RegisterAllocater::RegisterAllocater(std::shared_ptr<ControlFlowGraph> &cfg) : cfg_(cfg) {
  // 1. 初始化int_regs_和float_regs_
  for (auto reg = RegisterName::A0; reg <= RegisterName::A7; reg = static_cast<RegisterName>(reg + 1)) {
    int_a_regs_.push(reg);
  }
  // S0寄存器用于保存栈指针，因此不能用于分配
  for (auto reg = RegisterName::S1; reg <= RegisterName::S11; reg = static_cast<RegisterName>(reg + 1)) {
    int_s_regs_.push(reg);
  }
  for (auto reg = RegisterName::T0; reg <= RegisterName::T6; reg = static_cast<RegisterName>(reg + 1)) {
    int_t_regs_.push(reg);
  }
  for (auto reg = RegisterName::FA0; reg <= RegisterName::FA7; reg = static_cast<RegisterName>(reg + 1)) {
    float_a_regs_.push(reg);
  }
  for (auto reg = RegisterName::FS0; reg <= RegisterName::FS11; reg = static_cast<RegisterName>(reg + 1)) {
    float_s_regs_.push(reg);
  }
  for (auto reg = RegisterName::FT0; reg <= RegisterName::FT11; reg = static_cast<RegisterName>(reg + 1)) {
    float_t_regs_.push(reg);
  }
  // 2. 首先为函数参数分配寄存器和栈空间
  auto over_size_cnt_ = 0;
  for (auto &param: cfg->func_params_) {
    auto var = param->GetResultVar();
    // 如果是整型、指针类型或者布尔类型，则分配整型寄存器
    if (var->type_ == IRVariable::Type::INT || var->type_ == IRVariable::Type::INT_POINTER ||
        var->type_ == IRVariable::Type::FLOAT_POINTER || var->type_ == IRVariable::Type::BOOL) {
      if (int_a_regs_.empty()) {
        auto stk = new Stack(RegisterName::S0, over_size_cnt_ * 8);
        over_size_cnt_++;
        var->home_location_ = std::shared_ptr<Location>(stk);
      } else {
        var->home_location_ = std::shared_ptr<Register>(new Register(int_a_regs_.top()));
        name2ptr_[int_a_regs_.top()] = std::dynamic_pointer_cast<Register>(var->home_location_);
        int_a_regs_.pop();
      }
    } else {  // 如果是浮点型
      if (float_a_regs_.empty()) {
        auto stk = new Stack(RegisterName::S0, over_size_cnt_ * 8);
        over_size_cnt_++;
        var->home_location_ = std::shared_ptr<Location>(stk);
      } else {
        var->home_location_ = std::shared_ptr<Register>(new Register(float_a_regs_.top()));
        name2ptr_[float_a_regs_.top()] = std::dynamic_pointer_cast<Register>(var->home_location_);
        float_a_regs_.pop();
      }
    }
  }
}

auto RegisterAllocater::AllocateRegister() -> std::map<IRVariablePtr, std::shared_ptr<Location>> {
  auto cfg = cfg_;
  // 整型和浮点型的变量分开计算活跃区间和分配寄存器
  CalInterval(cfg);
  std::vector<IRVariablePtr> vars;  // 整型变量，包括int、指针、bool
  std::vector<IRVariablePtr> vars_f;  // 浮点型变量
  auto &var2interval = var2interval_;
  auto &var2interval_f = var2interval_f_;
  for (auto &var: var2interval) {
    vars.push_back(var.first);
  }
  for (auto &var: var2interval_f) {
    vars_f.push_back(var.first);
  }
  // 根据var2interval.first从小到大排序
  std::sort(vars.begin(), vars.end(), [&var2interval](const IRVariablePtr &a, const IRVariablePtr &b) {
      return var2interval[a].first < var2interval[b].first;
  });
  std::sort(vars_f.begin(), vars_f.end(), [&var2interval_f](const IRVariablePtr &a, const IRVariablePtr &b) {
      return var2interval_f[a].first < var2interval_f[b].first;
  });

  for (auto &var: vars) {
    ExpireOldVar(var);
    auto reg = GetFreeIntReg();
    if (reg == RegisterName::NONE) {
      // spill
      SpillAtInterval(var);
    } else {
      AddActiveVar(var);
      if (name2ptr_.find(reg) == name2ptr_.end()) {
        name2ptr_[reg] = std::shared_ptr<Register>(new Register(reg));
        var->home_location_ = name2ptr_[reg];
      } else {
        var->home_location_ = name2ptr_[reg];
      }
    }
  }

  for (auto &var: vars_f) {
    ExpireOldVar_f(var);
    auto reg = GetFreeFloatReg();
    if (reg == RegisterName::NONE) {
      // spill
      SpillAtInterval_f(var);
    } else {
      AddActiveVar_f(var);
      if (name2ptr_.find(reg) == name2ptr_.end()) {
        name2ptr_[reg] = std::shared_ptr<Register>(new Register(reg));
        var->home_location_ = name2ptr_[reg];
      } else {
        var->home_location_ = name2ptr_[reg];
      }
    }
  }

  return GetVar2Loc();
}

auto RegisterAllocater::ExpireOldVar(const IRVariablePtr &var_i) -> void {
  std::queue<IRVariablePtr> expired_vars;
  for (auto &var: active_vars_) {
    if (var2interval_[var].second >= var2interval_[var_i].first) {
      break;
    }
    expired_vars.push(var);
    // release a register to pool of free registers
    auto reg = std::dynamic_pointer_cast<Register>(var->home_location_)->reg_;
    if (reg <= RegisterName::T6) {
      int_t_regs_.push(reg);
    } else if (reg <= RegisterName::S11) {
      int_s_regs_.push(reg);
    } else if (reg <= RegisterName::A7) {
      int_a_regs_.push(reg);
    } else {
      throw std::runtime_error("Unknown Register");
    }
  }

  if (expired_vars.empty()) {
    return;
  }

  // remove expired vars from active_vars_
  // 删除过期变量，为了维护active_vars的有序性，即active_vars中的变量按照其活跃区间的右端点升序排序
  // 做法是未过期变量往前移，覆盖掉过期变量，然后删除末尾的无用变量
  int offset = 0;
  for (size_t j = 0; j < active_vars_.size(); j++) {
    auto &var = active_vars_[j];
    if (!expired_vars.empty() && var == expired_vars.front()) {
      expired_vars.pop();
      offset++;
    } else {
      active_vars_[j - offset] = active_vars_[j];
    }
  }
  while (offset--) {
    active_vars_.pop_back();
  }
}

auto RegisterAllocater::ExpireOldVar_f(const IRVariablePtr &var_i) -> void {
  std::queue<IRVariablePtr> expired_vars;
  for (auto &var: active_vars_f_) {
    if (var2interval_f_[var].second >= var2interval_f_[var_i].first) {
      break;
    }
    expired_vars.push(var);
    // release a register to pool of free registers
    auto reg = std::dynamic_pointer_cast<Register>(var->home_location_)->reg_;
    if (reg <= RegisterName::FT11) {
      float_t_regs_.push(reg);
    } else if (reg <= RegisterName::FS11) {
      float_s_regs_.push(reg);
    } else if (reg <= RegisterName::FA7) {
      float_a_regs_.push(reg);
    } else {
      throw std::runtime_error("Unknown Register");
    }
  }

  if (expired_vars.empty()) {
    return;
  }

  // remove expired vars from active_vars_
  // 删除过期变量，为了维护active_vars的有序性，即active_vars中的变量按照其活跃区间的右端点升序排序
  // 做法是未过期变量往前移，覆盖掉过期变量，然后删除末尾的无用变量
  int offset = 0;
  for (size_t j = 0; j < active_vars_f_.size(); j++) {
    auto &var = active_vars_f_[j];
    if (var == expired_vars.front()) {
      expired_vars.pop();
      offset++;
    } else {
      active_vars_f_[j - offset] = active_vars_f_[j];
    }
  }
  while (offset--) {
    active_vars_f_.pop_back();
  }
}

auto RegisterAllocater::AddActiveVar(const IRVariablePtr &var_i) -> void {
  for (size_t i = 0; i < active_vars_.size(); i++) {
    if (var2interval_[active_vars_[i]].second > var2interval_[var_i].second) {
      active_vars_.insert(active_vars_.begin() + i, var_i);
      return;
    }
  }
  active_vars_.push_back(var_i);
}

auto RegisterAllocater::AddActiveVar_f(const IRVariablePtr &var_i) -> void {
  for (size_t i = 0; i < active_vars_f_.size(); i++) {
    if (var2interval_f_[active_vars_f_[i]].second > var2interval_f_[var_i].second) {
      active_vars_f_.insert(active_vars_f_.begin() + i, var_i);
      return;
    }
  }
  active_vars_f_.push_back(var_i);
}

auto RegisterAllocater::SpillAtInterval(const IRVariablePtr &var_i) -> void {
  auto spill_var = active_vars_.back();
  if (var2interval_[spill_var].second > var2interval_[var_i].second) {
    active_vars_.pop_back();
    AddActiveVar(var_i);
    var_i->home_location_ = spill_var->home_location_;
    spill_var->home_location_ = std::shared_ptr<Location>(new Stack());
  } else {
    var_i->home_location_ = std::shared_ptr<Location>(new Stack());
  }
}

auto RegisterAllocater::SpillAtInterval_f(const IRVariablePtr &var_i) -> void {
  auto spill_var = active_vars_f_.back();
  if (var2interval_f_[spill_var].second > var2interval_f_[var_i].second) {
    active_vars_f_.pop_back();
    AddActiveVar(var_i);
    var_i->home_location_ = spill_var->home_location_;
    spill_var->home_location_ = std::shared_ptr<Location>(new Stack());
  } else {
    var_i->home_location_ = std::shared_ptr<Location>(new Stack());
  }
}

auto RegisterAllocater::GetVar2Loc() -> std::map<IRVariablePtr, std::shared_ptr<Location>> {
  std::map<IRVariablePtr, std::shared_ptr<Location>> var2loc;
  for (auto &var: var2interval_) {
    var2loc[var.first] = var.first->home_location_;
  }
  for (auto &var: var2interval_f_) {
    var2loc[var.first] = var.first->home_location_;
  }
  for (auto &param_ir : cfg_->func_params_) {
    auto var = param_ir->GetResultVar();
    var2loc[var] = var->home_location_;
  }
  return var2loc;
}

auto RegisterAllocater::GetFreeIntReg() -> RegisterName {
  if (!int_t_regs_.empty()) {
    auto reg = int_t_regs_.top();
    int_t_regs_.pop();
    return reg;
  } else if (!int_s_regs_.empty()) {
    auto reg = int_s_regs_.top();
    int_s_regs_.pop();
    return reg;
  } else if (!int_a_regs_.empty()) {
    auto reg = int_a_regs_.top();
    int_a_regs_.pop();
    return reg;
  } else {
    return RegisterName::NONE;
  }
}

auto RegisterAllocater::GetFreeFloatReg() -> RegisterName {
  if (!float_t_regs_.empty()) {
    auto reg = float_t_regs_.top();
    float_t_regs_.pop();
    return reg;
  } else if (!float_s_regs_.empty()) {
    auto reg = float_s_regs_.top();
    float_s_regs_.pop();
    return reg;
  } else if (!float_a_regs_.empty()) {
    auto reg = float_a_regs_.top();
    float_a_regs_.pop();
    return reg;
  } else {
    return RegisterName::NONE;
  }
}

// 现在这里不计算活跃区间了，只是没改名字，仅仅是为立即数分配location如果是立即数，则直接分配立即数
auto RegisterAllocater::InitImm(SCNUCC::IRVariablePtr &var) -> void {
  if (var->type_ == IRVariable::Type::INT_IMM || var->type_ == IRVariable::Type::BOOL_IMM) {
    auto imm32 = std::shared_ptr<Immediate32>(new Immediate32(var->int_value_, true));
    var->home_location_ = imm32;
  } else if (var->type_ == IRVariable::Type::FLOAT_IMM) {
    auto imm32 = std::shared_ptr<Immediate32>(
            new Immediate32(var->float_value_));
    var->home_location_ = imm32;
  }
}

// 现在这里不计算活跃区间了，只是没改名字，仅仅是为立即数分配location如果是立即数，则直接分配立即数
auto RegisterAllocater::UpdateInterval(SCNUCC::IRVariablePtr &var) -> void {
  if (var->name_[0] == '@') {
    return;
  }
  if (var->type_ == IRVariable::Type::INT || var->type_ == IRVariable::Type::INT_POINTER ||
      var->type_ == IRVariable::Type::BOOL || var->type_ == IRVariable::Type::FLOAT_POINTER) {
    // 如果是整型、指针类型或者布尔类型，则分配整型寄存器
    // var2interval_[var].second = ir_cnt_;
    // 这里先不管
  } else if (var->type_ == IRVariable::Type::FLOAT) {
    // 如果是浮点型
    // var2interval_f_[var].second = ir_cnt_;
    // 这里也先不管
  } else if (var->type_ == IRVariable::Type::INT_IMM || var->type_ == IRVariable::Type::BOOL_IMM) {
    auto imm32 = std::shared_ptr<Immediate32>(new Immediate32(var->int_value_, true));
    var->home_location_ = imm32;
  } else if (var->type_ == IRVariable::Type::FLOAT_IMM) {
    auto imm32 = std::shared_ptr<Immediate32>(
            new Immediate32(var->float_value_));
    var->home_location_ = imm32;
  }
}

auto RegisterAllocater::CalInterval(std::shared_ptr<ControlFlowGraph> &cfg) -> void {
  LiveAnalyzer live_analyzer;
  live_analyzer.cfg_ = cfg;
  auto tmp = live_analyzer.Analyze();
  var2interval_ = tmp.first;
  var2interval_f_ = tmp.second;

  for (auto &param: cfg->func_params_) {
    auto var = param->GetResultVar();
    var2interval_.erase(var);
    var2interval_f_.erase(var);
  }
  // 给imm分配一下空间
  for (auto &block: cfg->blocks_) {
    for (auto &stmt: block->stmts_) {
      if (std::dynamic_pointer_cast<BlockDeclIR>(stmt) || std::dynamic_pointer_cast<ConstantIR>(stmt)) {
        continue;
      } else if (auto br_ir = std::dynamic_pointer_cast<BRIR>(stmt)) {
        auto oprnd = br_ir->GetOprnds();
        if (oprnd.size() == 1) {  // 无条件跳转
          // do nothing
        } else {
          // BRIR的第一个操作数是条件，其余的操作数是跳转目标
          auto cond = br_ir->GetCond()->GetResultVar();
          InitImm(cond); // 如果是整型、指针类型或者布尔类型，则计算活跃区间；如果是立即数，则直接分配立即数
        }
      } else if (auto ret_ir = std::dynamic_pointer_cast<RetIR>(stmt)) {
        if (ret_ir->GetOperands().empty()) {
          continue;
        }
        auto var = ret_ir->GetOperands()[0];
        InitImm(var);
      } else if (std::dynamic_pointer_cast<ArrayDeclIR>(stmt)) {
        auto ir = std::dynamic_pointer_cast<ArrayDeclIR>(stmt);
        auto array_size = ir->GetArraySize();
        auto var = stmt->GetResultVar();
        var->array_location_ = std::shared_ptr<Stack>(new Stack(array_size * 2)); // 数组的初始化交给register manager
      } else if (std::dynamic_pointer_cast<ArithmeticIR>(stmt) ||
                 std::dynamic_pointer_cast<CmpIR>(stmt)) {
        auto var = stmt->GetResultVar();
        for (auto &oprnd: stmt->GetOperands()) {
          InitImm(oprnd);
        }
      } else if (std::dynamic_pointer_cast<AllocaIR>(stmt)) {
        throw "alloc not used";
      } else if (std::dynamic_pointer_cast<LoadIR>(stmt)) {
        auto var = stmt->GetResultVar();
        InitImm(stmt->GetOperands()[0]);
      } else if (std::dynamic_pointer_cast<StoreIR>(stmt)) {
        auto var = stmt->GetOperands()[0];
        InitImm(var);
      } else if (std::dynamic_pointer_cast<PhiIR>(stmt)) {
        auto var = stmt->GetResultVar();
        // 
        for (size_t i = 0; i < stmt->GetOprndSize(); i += 2) { // 偶数的oprnd才是变量，基数是block label
          var = stmt->GetOperands()[i];
          if (var->type_ == IRVariable::Type::INT_IMM || var->type_ == IRVariable::Type::BOOL_IMM) {
            auto imm32 = std::shared_ptr<Immediate32>(new Immediate32(var->int_value_, true));
            var->home_location_ = imm32;
          } else if (var->type_ == IRVariable::Type::FLOAT_IMM) {
            auto imm32 = std::shared_ptr<Immediate32>(
                    new Immediate32(var->float_value_));
            var->home_location_ = imm32;
          }
        }
      } else if (std::dynamic_pointer_cast<CallIR>(stmt)) {
        auto var = stmt->GetResultVar();
        for (auto &oprnd: stmt->GetOperands()) {
          InitImm(oprnd);
        }
      } else if (std::dynamic_pointer_cast<IntToFloatIR>(stmt) ||
                 std::dynamic_pointer_cast<FloatToIntIR>(stmt) ||
                 std::dynamic_pointer_cast<BoolToIntIR>(stmt)) {
        auto var = stmt->GetResultVar();
        InitImm(var);
      } else if (std::dynamic_pointer_cast<LogicIR>(stmt)) {
        auto var = stmt->GetResultVar();
        InitImm(var);
      } else if (std::dynamic_pointer_cast<BitCastIR>(stmt)) {
        // do nothing
      } else if (std::dynamic_pointer_cast<GEPIR>(stmt)) {
        auto var = stmt->GetResultVar();
        auto oprnds = stmt->GetOperands();
        InitImm(oprnds[0]);
        InitImm(oprnds[1]);
      } else {
        throw std::runtime_error("Unknown IR type: " + stmt->ToSSA());
      }
    }
  }
  for (auto &block: cfg_->blocks_) {
    for (auto &stmt: block->stmts_) {
      if (std::dynamic_pointer_cast<PhiIR>(stmt)) {
        auto phi_var = stmt->GetResultVar();
        if (phi_var->type_ == IRVariable::Type::INT_POINTER || phi_var->type_ == IRVariable::Type::FLOAT_POINTER) {
          auto uses_oprand = stmt->GetOperands();
          for (size_t i = 0; i < uses_oprand.size(); i += 2) {
            if ((uses_oprand[i]->type_ == IRVariable::Type::INT_POINTER || uses_oprand[i]->type_ == IRVariable::Type::FLOAT_POINTER)
                && uses_oprand[i]->array_location_ != nullptr) {
              phi_var->array_location_ = uses_oprand[i]->array_location_;
            }
          }
        } 
      }
    }
  }
}

auto RegisterAllocater::ToString() -> std::string {
  std::stringstream os;
  for (auto &var: var2interval_) {
    os << var.first->name_ << " -> " << var.first->home_location_->name_ << " (" << var.second.first << ", "
       << var.second.second << ")\n";
  }
  for (auto &var: var2interval_f_) {
    os << var.first->name_ << " -> " << var.first->home_location_->name_ << " (" << var.second.first << ", "
       << var.second.second << ")\n";
  }
  return os.str();
}


} // namespace SCNUCC

