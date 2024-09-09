
/**
 * 包含
 * Parall Copy 类，用于消除Phi函数
 * AsmBlock类，存放RISCV汇编代码的代码块
 */
#ifndef SCNUCC_ASMBLOCK_H
#define SCNUCC_ASMBLOCK_H

#include <vector>
#include <string>
#include <set>
#include <memory>
#include "./riscv.h"

namespace SCNUCC {
class AsmInstruction;

class RegisterManager;

class PhiIR;

class Block;

class Location;

using AsmInstrPtr = std::shared_ptr<AsmInstruction>;
using AsmInstrPtrVec = std::vector<AsmInstrPtr>;
/**
 * Parall Copy 类，用于消除Phi函数
 */
class ParallelCopy {
public:
  // pair<src, dest>
  std::set<std::pair<std::shared_ptr<Location>, std::shared_ptr<Location>>> copies_;
  RegisterManager *reg_manager_;

  ParallelCopy(RegisterManager *registerManager) {
    reg_manager_ = registerManager;
  }

  auto AddCopy(std::shared_ptr<Location> &src, std::shared_ptr<Location> &dst) -> void;

  auto FindNoCycleCopy() -> std::pair<std::shared_ptr<Location>, std::shared_ptr<Location>>;

  auto
  Copy2Instr(const std::shared_ptr<Location> &src, const std::shared_ptr<Location> &dst, AsmInstrPtrVec &res) -> void;

  auto GenerateCopyInstr() -> AsmInstrPtrVec;
};

/**
 * AsmBlock类，存放RISCV汇编代码的代码块
 */
class AsmBlock {
public:
  using AsmBlockPtr = std::shared_ptr<AsmBlock>;
  std::vector<AsmInstrPtr> instructions_;
  std::vector<std::shared_ptr<PhiIR>> phi_irs_;
  std::set<AsmBlockPtr> parents_;
  std::string block_name_;
  std::shared_ptr<Block> ir_blk_;
  std::shared_ptr<ParallelCopy> parallel_copy_;
  RegisterName jump_cond_;
  AsmBlockPtr true_br_;
  AsmBlockPtr false_br_;
  int block_flag_;  // 1是入口block，2是retblock，其他值不管

public:
  AsmBlock() = default;

  explicit AsmBlock(const std::string &block_name, RegisterManager *regm) : block_name_(block_name) {
    parallel_copy_ = std::make_shared<ParallelCopy>(regm);
    instructions_.resize(0);
    phi_irs_.resize(0);
    block_flag_ = 0;
  }

  void SetRegManager(RegisterManager *regm) {
    parallel_copy_->reg_manager_ = regm;
  }

  void AddInstruction(const AsmInstrPtr &instr) { instructions_.push_back(instr); }

  void AddParent(const AsmBlockPtr &parent) {
    for (const auto& p : parents_) {
      if (parent == p) {
        return;
      }
    }
    parents_.insert(parent);
  }

  void AddPhi(const std::shared_ptr<PhiIR> &phi) { phi_irs_.push_back(phi); }

  auto GetParents() const -> const std::set<AsmBlockPtr> & { return parents_; }

  auto GetChildren() const -> std::pair<AsmBlockPtr, AsmBlockPtr>  {
    return {true_br_, false_br_};
  }

  auto GetChildNum() const -> size_t  {
    size_t res = 0;
    if (true_br_) {
      res += 1;
    }
    if (false_br_) {
      res += 1;
    }
    return res;
  }

  auto GetParallelCopy() -> std::shared_ptr<ParallelCopy> { return parallel_copy_; }

  auto GetName() -> std::string { return block_name_; }

  std::string ToString(const std::string &allocate, const std::string &destroy);

  auto SetJumpCond(RegisterName cond) -> void;

  auto SetTrueBranch(const AsmBlockPtr &true_br) -> void;

  auto SetFalseBranch(const AsmBlockPtr &false_br) -> void;

  auto ChangeBranch(const AsmBlockPtr &old, const AsmBlockPtr &neww) -> void;
};

}  // namespace SCNUCC

#endif //SCNUCC_ASMBLOCK_H
