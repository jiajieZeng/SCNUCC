//
//  .
//

#ifndef SCNUCC_BLOCK_H
#define SCNUCC_BLOCK_H

#include <vector>
#include <map>
#include <set>
#include <string>
#include <memory>
#include <any>

#include "./SymbolTable.h"

namespace SCNUCC {

namespace BLOCK {
enum BLOCK {
  NO_RETURN,
  HAS_RETURN,
  HAS_CONTINUE,
  HAS_BREAK
};
}

class Scope;

class IRStatement;

class PhiIR;

class SymbolTable;

class Block : public std::enable_shared_from_this<Block> {
public:
  friend class AsmGenerator;
  friend class RegisterAllocater;
  // 工厂模式
  static std::shared_ptr<Block> Create(std::shared_ptr<SymbolTable> sym_table, std::shared_ptr<Block> while_entry,
        std::shared_ptr<Block> while_exit) {
    auto instance = std::shared_ptr<Block>(new Block(sym_table, while_entry, while_exit));
    instance->InitBlockDeclIRWithBlock();
    return instance;
  }

  Block(std::shared_ptr<SymbolTable> sym_table, std::shared_ptr<Block> while_entry,
        std::shared_ptr<Block> while_exit);

  ~Block() = default;

  void InitBlockDeclIRWithBlock();

  void AddStatement(std::shared_ptr<IRStatement> stmt);

  void AddChild(std::shared_ptr<Block> child);

  void AddParent(std::shared_ptr<Block> parent);

  auto GetParent() const -> const std::vector<std::shared_ptr<Block>> &;

  auto GetChildren() const -> const std::vector<std::shared_ptr<Block>> &;

  virtual auto ToSSA() const -> std::string;

  auto NewPhi(const std::shared_ptr<VarSymbol> &var) -> std::shared_ptr<PhiIR>;

  auto AddPhiOperands(const std::shared_ptr<VarSymbol> &var, const std::shared_ptr<PhiIR> &phi) -> void;

  auto TryRemoveTrivialPhi(const std::shared_ptr<PhiIR> &phi) -> void;

  auto ReadVariable(const std::shared_ptr<VarSymbol> &var) -> std::shared_ptr<IRStatement>;

  auto WriteVariable(const std::shared_ptr<VarSymbol> &var, const std::shared_ptr<IRStatement> &stmt) -> void;

  auto IsSealed() const -> bool;

  auto Seal() -> void;

  auto InsertIRs(const std::vector<std::shared_ptr<IRStatement>> &stmts) -> void;

  static size_t block_cnt_;

  virtual auto IsGblInitBlock() const -> bool;

  auto GetStmts() const -> const std::vector<std::shared_ptr<IRStatement>> &;

  virtual auto GetBlockID() const -> size_t;

  virtual auto SetSequenceID(size_t id) -> void;

  virtual auto GetSequenceID() const -> size_t;

  /**
   * 调整block内部语句的顺序，吧phi全都放到开头
  */
  auto AdjustID(size_t &ir_sequence_id) -> void;

  /**
   * return_block_专用，扫描所有的父亲节点，如果PhiIR不包含来自这个父亲节点的操作书，添加一个返回未定义的值
   */
  auto AddReturnVal(std::shared_ptr<PhiIR> &phi, IRTYPE::IRTYPE ret_type) -> void;

  virtual auto SetDefUse() -> void;

  size_t begin_numbering_;  // 块内第一条语句的numbering
  size_t end_numbering_;    // 块内最后一条语句的numbering

protected:
  std::map<std::shared_ptr<VarSymbol>, std::shared_ptr<PhiIR>> incomplete_phis_;

  std::vector<std::shared_ptr<IRStatement>> stmts_;

  std::map<std::shared_ptr<VarSymbol>, std::shared_ptr<IRStatement>> var2def_;

  std::vector<std::shared_ptr<Block>> children_;

  std::vector<std::shared_ptr<Block>> parent_;

  size_t block_id_;

  std::shared_ptr<Block> while_entry_;

  std::shared_ptr<Block> while_exit_;

  bool is_sealed = false;

  std::shared_ptr<SymbolTable> sym_table_;

  size_t sequence_id_;  // block在cfg里面的id


};


/**
 * 用于初始化全局变量的Block
 * 每一个全局变量使用一个GblVarDeclIR定义，然后使用GblInitBlock初始化
 * 比如：
 * int a = 1;
 * int b = a + 1;
 * 那么b的GblInitBlock可以写成：
 * define internal void @init_b() #0 section ".text.startup" {
 *   %1 = load i32, ptr @a, align 4
 *   %2 = add nsw i32 %1, 1
 *   store i32 %2, ptr @b, align 4
 *   ret void
 * }
 */
class GblInitBlock : public Block {
public:
  GblInitBlock();

  auto IsGblInitBlock() const -> bool override;

  auto ToSSA() const -> std::string override;

  auto AdjustID() -> void;

  auto SetDefUse() -> void override;
};

} // SCNUCC

#endif //SCNUCC_BLOCK_H
