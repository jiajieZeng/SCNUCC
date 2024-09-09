//
//  .
//

#ifndef SCNUCC_CONTROLFLOWGRAPH_H
#define SCNUCC_CONTROLFLOWGRAPH_H

#include <vector>
#include <memory>
#include <any>
#include "./Block.h"
#include "./SymbolTable.h"
#include "./LLVMIR.h"


namespace SCNUCC {

class Scope;

class SymbolTable;

class FuncParamIR;

class ControlFlowGraph {
public:
  friend class AsmGenerator;
  friend class RegisterAllocater;
  friend class LiveAnalyzer;
  ControlFlowGraph(std::shared_ptr<SymbolTable> sym_table, std::string func_name);

  ~ControlFlowGraph() = default;

  void Concat(const std::shared_ptr<ControlFlowGraph> &cfg);

  void Concat(const std::shared_ptr<Block> &block);

  void AddBlock(const std::shared_ptr<Block> &block);

  auto GetLastBlock() const -> const std::shared_ptr<Block> &;

  auto AddParam(const std::shared_ptr<FuncParamIR> &param) -> void;

  // TODO: 使用BFS遍历CFG，生成SSA形式的字符串
  auto ToSSA() const -> std::string;

  auto BlockNum() const -> size_t;

  static auto ConnectBlocks(const std::shared_ptr<Block> &from, const std::shared_ptr<Block> &to) -> void;

  auto AdjustID() -> void;

  auto SetDefUse() -> void;

  auto DFSTopology() -> std::vector<std::shared_ptr<Block>>;

  auto AddBackEdge(const std::shared_ptr<Block> &from, const std::shared_ptr<Block> &to) -> void;

  auto IsBackEdge(const std::shared_ptr<Block> &from, const std::shared_ptr<Block> &to) -> bool;

private:
  std::set<std::pair<std::shared_ptr<Block>, std::shared_ptr<Block>>> back_edge_;

  auto DFSTopology(std::shared_ptr<Block> &cur_blk, std::vector<std::shared_ptr<Block>> &result, std::set<std::shared_ptr<Block>> &visited) -> void;

  static auto BaseTypeToIRType(BaseType base_type) -> std::string;

  std::vector<std::shared_ptr<Block>> blocks_;

  std::shared_ptr<SymbolTable> sym_table_;

  std::string func_name_;

  std::vector<std::shared_ptr<FuncParamIR>> func_params_;
  
};
}
#endif //SCNUCC_CONTROLFLOWGRAPH_H
