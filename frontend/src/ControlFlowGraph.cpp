//
//  .
//
#include <sstream>
#include <algorithm>
#include "../include/ControlFlowGraph.h"
#include "../include/SymbolTable.h"
#include "../include/IRStatement.h"

namespace SCNUCC {
ControlFlowGraph::ControlFlowGraph(std::shared_ptr<SymbolTable> sym_table, std::string func_name) : sym_table_(
        std::move(sym_table)), func_name_(std::move(func_name)) {
}

void ControlFlowGraph::Concat(const std::shared_ptr<ControlFlowGraph> &cfg) {
  blocks_[blocks_.size() - 1]->AddChild(cfg->blocks_[0]);
  cfg->blocks_[0]->AddParent(blocks_[blocks_.size() - 1]);
  for (const auto &block: cfg->blocks_) {
    blocks_.push_back(block);
  }
}

auto ControlFlowGraph::ToSSA() const -> std::string {
  std::stringstream ssa;

  auto func_symbol = std::dynamic_pointer_cast<FuncSymbol>(sym_table_->FuncLookUp(func_name_));
  if (!func_symbol) {
    throw std::runtime_error("Function not found");
  }
  std::string type = BaseTypeToIRType(func_symbol->GetReturnType());
  ssa << "define " << type << " @" << func_name_;
  ssa << "( ";
  for (int i = 0; i < func_symbol->ParamSize(); i++) {
    auto param = func_symbol->GetAllParams()[i];
    if (param.base_type_ == BaseType::INT) {
      if (param.array_shape_.size()) {
        ssa << "i32*";
      } else {
        ssa << "i32";
      }
    } else {
      if (param.array_shape_.size()) {
        ssa << "float*";
      } else {
        ssa << "float";
      }
    }
    ssa << " %" << i;
    if (i != func_symbol->ParamSize() - 1) {
      ssa << ", ";
    }
  }
  if (func_symbol->ParamSize()) {
    ssa << " ";
  }
  ssa << ") {\n";

  for (const auto &block: blocks_) {
    ssa << block->ToSSA();
  }
  ssa << "}\n";
  return ssa.str();
}


void ControlFlowGraph::Concat(const std::shared_ptr<Block> &block) {
  if (blocks_.empty()) {
    block->SetSequenceID(0);
    blocks_.push_back(block);
    return;
  }
  size_t id = blocks_.size();
  blocks_[id - 1]->AddChild(block);
  block->AddParent(blocks_[id - 1]);
  block->SetSequenceID(id);
  blocks_.push_back(block);
}

void ControlFlowGraph::AddBlock(const std::shared_ptr<Block> &block) {
  size_t id = blocks_.size();
  block->SetSequenceID(id);
  blocks_.push_back(block);
}

void ControlFlowGraph::ConnectBlocks(const std::shared_ptr<Block> &from, const std::shared_ptr<Block> &to) {
  from->AddChild(to);
  to->AddParent(from);
}

auto ControlFlowGraph::BaseTypeToIRType(BaseType base_type) -> std::string {
  switch (base_type) {
    case BaseType::INT:
      return "i32";
    case BaseType::FLOAT:
      return "float";
    case BaseType::VOID:
      return "void";
    default:
      throw std::runtime_error("Unsupported type");
  }
}

auto ControlFlowGraph::GetLastBlock() const -> const std::shared_ptr<Block> & {
  return blocks_.back();
}

auto ControlFlowGraph::BlockNum() const -> size_t {
  return blocks_.size();
}

auto ControlFlowGraph::AdjustID() -> void {
  auto func_symbol = std::dynamic_pointer_cast<FuncSymbol>(sym_table_->FuncLookUp(func_name_));
  if (!func_symbol) {
    throw std::runtime_error("Function not found");
  }
  // 从参数的后一个开始
  size_t ir_sequence_id = func_symbol->ParamSize();
  // 先调整一遍
  size_t blocks_len = blocks_.size();
  if (blocks_len) {
    for (int i = 0; i < blocks_len; i++) {
      blocks_[i]->AdjustID(ir_sequence_id); // 重新编号
    }
  }
  auto blocks_list = DFSTopology();
  std::reverse(blocks_list.begin(), blocks_list.end());
  size_t numbering = 0;
  for (auto &block: blocks_list) {
    block->begin_numbering_ = numbering;
    auto stmts = block->GetStmts();
    for (auto ir: stmts) {
      ir->numbering = numbering++;
    }
    block->end_numbering_ = numbering - 1;
  }
}

auto ControlFlowGraph::SetDefUse() -> void {
  for (auto &block: blocks_) {
    block->SetDefUse();
  }
}

auto ControlFlowGraph::AddParam(const std::shared_ptr<FuncParamIR> &param) -> void {
  func_params_.push_back(param);
}

auto ControlFlowGraph::AddBackEdge(const std::shared_ptr<Block> &from, const std::shared_ptr<Block> &to) -> void {
  back_edge_.insert(std::make_pair(from, to));
}

auto ControlFlowGraph::IsBackEdge(const std::shared_ptr<Block> &from, const std::shared_ptr<Block> &to) -> bool {
  return back_edge_.find(std::make_pair(from, to)) != back_edge_.end();
}

auto ControlFlowGraph::DFSTopology() -> std::vector<std::shared_ptr<Block>> {
  std::vector<std::shared_ptr<Block>> result;
  std::set<std::shared_ptr<Block>> visited;
  DFSTopology(blocks_[0], result, visited);
  return result;
}

auto ControlFlowGraph::DFSTopology(std::shared_ptr<Block> &cur_blk, std::vector<std::shared_ptr<Block>> &result, std::set<std::shared_ptr<Block>> &visited) -> void {
  visited.insert(cur_blk);
  auto children = cur_blk->GetChildren();
  for (auto child: children) {
    if (visited.find(child) == visited.end()) {
      DFSTopology(child, result, visited);
    }
  }
  result.push_back(cur_blk);
}

} // SCNUCC