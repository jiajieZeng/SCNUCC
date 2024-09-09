
/**
 * 定义RegisterAllocator类
 * 进行线性扫描分配寄存器, 为变量分配空间，
 */

#ifndef SCNUCC_REGMANAGER_H
#define SCNUCC_REGMANAGER_H

#include <memory>
#include <map>
#include <vector>
#include <queue>
#include <memory>
#include <algorithm>
#include <functional>
#include "./riscv.h"


namespace SCNUCC {
class ControlFlowGraph;

class IRStatement;

class Location;

class IRVariable;

class Register;

typedef std::shared_ptr<Register> RegisterPtr;

typedef std::shared_ptr<IRVariable> IRVariablePtr;
using IRPtr = std::shared_ptr<IRStatement>;
using Interval = std::pair<size_t, size_t>;


class RegisterAllocater {
private:
  std::priority_queue<RegisterName, std::vector<RegisterName>, std::greater<int>> int_a_regs_;
  std::priority_queue<RegisterName, std::vector<RegisterName>, std::greater<int>> int_s_regs_;
  std::priority_queue<RegisterName, std::vector<RegisterName>, std::greater<int>> int_t_regs_;
  std::priority_queue<RegisterName, std::vector<RegisterName>, std::greater<int>> float_a_regs_;
  std::priority_queue<RegisterName, std::vector<RegisterName>, std::greater<int>> float_s_regs_;
  std::priority_queue<RegisterName, std::vector<RegisterName>, std::greater<int>> float_t_regs_;

  std::map<RegisterName, std::shared_ptr<Register>> name2ptr_;

  std::vector<IRVariablePtr> active_vars_;
  std::vector<IRVariablePtr> active_vars_f_;
  std::map<IRVariablePtr, Interval> var2interval_;
  std::map<IRVariablePtr, Interval> var2interval_f_;

  std::shared_ptr<ControlFlowGraph> cfg_;

  size_t ir_cnt_ = 1;

  auto CalInterval(std::shared_ptr<ControlFlowGraph> &cfg) -> void;

  auto ExpireOldVar(const IRVariablePtr &) -> void;

  auto ExpireOldVar_f(const IRVariablePtr &) -> void;

  auto SpillAtInterval(const IRVariablePtr &) -> void;

  auto SpillAtInterval_f(const IRVariablePtr &) -> void;

  auto AddActiveVar(const IRVariablePtr &) -> void;

  auto AddActiveVar_f(const IRVariablePtr &) -> void;

  auto GetVar2Loc() -> std::map<IRVariablePtr, std::shared_ptr<Location>>;

  auto GetFreeIntReg() -> RegisterName;

  auto GetFreeFloatReg() -> RegisterName;

  auto UpdateInterval(IRVariablePtr &var) -> void;

  auto InitImm(IRVariablePtr &var) -> void;

public:

  explicit RegisterAllocater(std::shared_ptr<ControlFlowGraph> &cfg);

  auto AllocateRegister() -> std::map<IRVariablePtr, std::shared_ptr<Location>>;

  auto ToString() -> std::string;
};

} // namespace SCNUCC

#endif //SCNUCC_REGMANAGER_H
