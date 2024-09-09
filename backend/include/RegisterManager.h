
/**
 * 定义RegisterManager类，主要在生成汇编代码的过程中从事寄存器的抢占，分配，
 * 处理Caller-save函数，Callee-save函数，生成函数调用的过程
 */
#ifndef SCNUCC_REGISTERMANAGER_H
#define SCNUCC_REGISTERMANAGER_H

#include <vector>
#include <map>
#include <set>
#include <queue>
#include <memory>
#include "./riscv.h"

namespace SCNUCC {
class IRStatement;

class CallIR;

class ArrayDeclIR;

using IRPtr = std::shared_ptr<IRStatement>;

class AsmGenerator;

class AsmInstruction;

class AsmBlock;

class Location;

class Register;

class Stack;

class IRVariable;

typedef std::shared_ptr<IRVariable> IRVariablePtr;
typedef std::shared_ptr<Register> RegisterPtr;
typedef std::shared_ptr<Stack> StackPtr;

class RegisterManager;

// 支持删除任意元素的优先队列
// https://blog.csdn.net/qq_38844835/article/details/125700807
class MyQueue {
private:
  struct RegisterCmp {
    bool operator()(const RegisterPtr &a, const RegisterPtr &b) const;
  };

  std::set<RegisterPtr, RegisterCmp> regs_;
public:
  auto push(const RegisterPtr &reg) -> void;

  auto pop() -> void;

  auto top() -> RegisterPtr;

  auto empty() -> bool;

  auto erase(const RegisterPtr &reg) -> void;
};

class RegisterManager {
private:
  std::map<IRVariablePtr, std::shared_ptr<Location>> var2location_;

  std::shared_ptr<AsmBlock> cur_blk_;

  // 包括了没有使用的寄存器
  MyQueue empty_int_regs_;
  MyQueue empty_float_regs_;
  // 包括了使用了但是没有被pin的寄存器
  MyQueue evictable_int_regs_;
  MyQueue evictable_float_regs_;

  struct StackCmp {
    bool operator()(StackPtr &a, StackPtr &b);
  };

  std::priority_queue<StackPtr, std::vector<StackPtr>, StackCmp> available_stk_;

  std::map<RegisterName, RegisterPtr> name2ptr_;

  std::map<RegisterName, StackPtr> callee_saved_to_; // 记录callee-save-registers保存到哪里
  std::map<RegisterName, StackPtr> caller_saved_to_; // 记录caller-save-registers保存到哪里
  std::set<RegisterName> used_t_regs_;
  std::set<RegisterName> used_s_regs_;
  std::set<RegisterName> used_a_regs_;
  std::set<RegisterName> used_ft_regs_;
  std::set<RegisterName> used_fs_regs_;
  std::set<RegisterName> used_fa_regs_;

  size_t max_args_size_ = 0;


public:

  explicit RegisterManager(std::map<IRVariablePtr, std::shared_ptr<Location>> &var2location,
                           std::shared_ptr<AsmBlock> &cur_blk);

  auto ReleaseStack(const StackPtr &stk) -> void;

  auto GetFreeStack() -> StackPtr;

  /**
   * 设置当前的Block，由AsmGenerator调用
   * RegisterManager会把生成的load和store指令添加到这个Block里面
   * @param blk
   */
  auto SetCurBlock(std::shared_ptr<AsmBlock> &blk) -> void;

  /**
   * 为IR中的每一条每一个运算数分配寄存器
   * 分为下面的情况：
   * 1. 如果一个运算数var在LinearScan的时候分配到了寄存器reg，那么AssignRegister直接为var分配reg
   * 2. 如果一个运算数var在LinearScan的时候分配到了栈空间stk，那么AssignRegister会为var分配一个新的寄存器，
   *    并且在这个过程中生成load和store指令把stk中的值load到这个寄存器中
   * 3. 如果一个运算数var是一个立即数，那么AssignRegister会为var分配一个新的寄存器，并且生成li指令把这个立即数load到这个寄存器中
   *
   * @param ir
   * @return
   */
  auto AssignRegister(std::vector<IRVariablePtr> &vars) -> std::vector<RegisterPtr>;

  /**
   * 获取一个新的整数寄存器
   * 需要注意的是，在返回的Register的时候，需要指定register的new_data_home_
   * @return
   */
  auto GetFreeIntReg() -> RegisterPtr;

  /**
 * 获取一个新的浮点寄存器
 * 需要注意的是，在返回的Register的时候，需要指定register的new_data_home_
 * @return
 */
  auto GetFreeFloatReg() -> RegisterPtr;

  /**
   * 释放一个寄存器
   * 需要注意的是，在释放的Register之前，需要保证已经设置了Register的new_data_home_
   * @param reg
   */
  auto ReleaseRegister(const RegisterPtr &reg) -> void;

  /**
   * 释放一个多个寄存器
   * @param reg
   */
  auto ReleaseRegister(const std::vector<RegisterPtr> &reg) -> void;

  auto NewStackLocation() -> std::shared_ptr<Stack>;

  auto GetSpecRegister(RegisterName) -> RegisterPtr;

  auto CallerSaveRegisters(std::shared_ptr<CallIR> &call_ir) -> void;

  auto CallerLoadRegisters(std::shared_ptr<CallIR> &call_ir) -> void;

  auto CalleeLoadRegisters() -> void;

  auto SolveFuncCalling(std::shared_ptr<CallIR> &call_ir) -> void;

  auto AlignSPWith16Byte() -> void;

  auto SolveMemset(std::shared_ptr<ArrayDeclIR> &array_decl) -> void;
};
}


#endif //SCNUCC_REGISTERMANAGER_H
