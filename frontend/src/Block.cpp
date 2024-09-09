#include <utility>
#include <sstream>
#include <memory>
#include <algorithm>

#include "../include/Block.h"
#include "../include/IRStatement.h"
#include "../include/SymbolTable.h"

namespace SCNUCC {
size_t Block::block_cnt_ = 0;

Block::Block(std::shared_ptr<SymbolTable> sym_table, std::shared_ptr<Block> while_entry,
             std::shared_ptr<Block> while_exit) :
        sym_table_(std::move(sym_table)), while_entry_(std::move(while_entry)), while_exit_(std::move(while_exit)),
        block_id_(block_cnt_++) {
  // 原本的写法会出错，我也不知道为什么换成下面的写法就不会出错了
  // std::shared_ptr<BlockDeclIR> blockDeclIR(new BlockDeclIR(block_id_));
  auto blockDeclIR = BlockDeclIR::Create(block_id_);
  // auto blockDeclIR = std::make_shared<BlockDeclIR>(block_id_);
  this->stmts_.push_back(blockDeclIR);
  this->is_sealed = false;
}

void Block::InitBlockDeclIRWithBlock() {
  auto decl = std::dynamic_pointer_cast<BlockDeclIR>(stmts_[0]);
  decl->blk_ = shared_from_this();
}

void Block::AddStatement(std::shared_ptr<IRStatement> stmt) {
  if (std::dynamic_pointer_cast<ConstantIR>(stmt)) {
    // ConstantIR 不能加入到stmts_中
    return;
  }
  stmts_.push_back(std::move(stmt));
}

void Block::AddChild(std::shared_ptr<Block> child) {
  if (std::count(children_.begin(), children_.end(), child)) {  // 避免重复添加
    return;
  }
  children_.push_back(std::move(child));
}

void Block::AddParent(std::shared_ptr<Block> parent) {
  if (std::count(parent_.begin(), parent_.end(), parent)) { // 避免重复添加
    return;
  }
  parent_.push_back(std::move(parent));
}

auto Block::ToSSA() const -> std::string {
  std::stringstream ssa;
  std::string indent = "    ";
  // ssa << stmts_[0]->numbering << ":  " << stmts_[0]->ToSSA() << "\n";
  ssa << stmts_[0]->ToSSA() << "\n";
  for (size_t i = 1; i < stmts_.size(); i++) {
    if (std::dynamic_pointer_cast<FuncParamIR>(stmts_[i])) {
    }
    if (std::dynamic_pointer_cast<ConstantIR>(stmts_[i])) {
      // ConstantIR 不能输出
      continue;
    }
    // ssa << stmts_[i]->numbering << indent << stmts_[i]->ToSSA() << "\n";
    ssa << indent << stmts_[i]->ToSSA() << "\n";
  }
  return ssa.str();
}

auto Block::GetParent() const -> const std::vector<std::shared_ptr<Block>> & {
  return parent_;
}

auto Block::GetChildren() const -> const std::vector<std::shared_ptr<Block>> & {
  return children_;
}

auto Block::NewPhi(const std::shared_ptr<VarSymbol> &var) -> std::shared_ptr<PhiIR> {
  IRTYPE::IRTYPE type;
  // 当前作用域找不到的话就要到父作用域去找，所以这里是ALL_SCOPE
  // auto c_type = sym_table_->GetCurrentScope()->VarLookUp(var, SearchScope::ALL_SCOPE)->GetType().base_type_;
  auto c_type = var->GetType().base_type_;
  if (c_type == BaseType::INT) {
    if (var->IsPtr()) {
      type = IRTYPE::IR_INT_PTR;
    } else {
      type = IRTYPE::IR_I32;
    }
  } else if (c_type == BaseType::FLOAT) {
    if (var->IsPtr()) {
      type = IRTYPE::IR_FLOAT_PTR;
    } else {
      type = IRTYPE::IR_FLOAT;
    }
  } else {
    throw std::runtime_error("Unsupported type");
  }

  // auto phi = std::make_shared<PhiIR>(type);
  auto phi = PhiIR::Create(type);
  if (std::dynamic_pointer_cast<BRIR>(stmts_.back())) {
    stmts_.insert(stmts_.end() - 1, phi);
  } else {
    stmts_.push_back(phi);
  }
  var2def_[var] = phi;
  return phi;
}

auto Block::AddPhiOperands(const std::shared_ptr<VarSymbol> &var, const std::shared_ptr<PhiIR> &phi) -> void {
  for (const auto &parent: parent_) {
    auto val = parent->ReadVariable(var);
    phi->AddOprnd(val);
    phi->AddOprnd(parent->GetStmts()[0]);
  }
  // TryRemoveTrivialPhi(phi);
}

auto ReplacePhi(std::shared_ptr<IRVariable> &phi, std::shared_ptr<IRVariable> &same) {
  auto users = phi->used_by_;
  for (auto &user: users) {
    auto &operands = user->defined_in_->GetOperands();
    for (size_t i = 0; i < operands.size(); i++) {
      if (operands[i] == phi) {
        operands[i] = same;
        user->defined_in_->oprnds_[i] = same->defined_in_;
        user->defined_in_->LinkDefUse(same->defined_in_);
      }
    }
  }
}

auto Block::TryRemoveTrivialPhi(const std::shared_ptr<PhiIR> &phi) -> void {
  std::shared_ptr<IRVariable> same_oprnd = nullptr, same_block = nullptr;
  auto oprnds = phi->GetOperands();
  size_t n = oprnds.size();
  if(n == 2) {
    // trivial phi
    ReplacePhi(phi->GetResultVar(), oprnds[0]);
    stmts_.erase(std::remove(stmts_.begin(), stmts_.end(), phi), stmts_.end());
    return;
  }
  for (size_t i = 0; i < n; i += 2) {
    if (same_oprnd == oprnds[i] && same_block == oprnds[i + 1]) {
      continue; // unique value or self-reference
    }
    if (same_oprnd != nullptr) {
      // return phi
      return; // the phi merges at least two values: not trivial
    }
    same_oprnd = oprnds[i];
    same_block = oprnds[i + 1];
  }
  // 这里应该是要删掉的意思
  if (same_oprnd == nullptr) {
    // should never happen
    throw std::runtime_error("Phi has no operands");
  }

  auto users = same_oprnd->used_by_;
  users.erase(std::remove(users.begin(), users.end(), phi->GetResultVar()), users.end());
  ReplacePhi(phi->GetResultVar(), same_oprnd);
  stmts_.erase(std::remove(stmts_.begin(), stmts_.end(), phi), stmts_.end());

  for (auto &user: users) {
    if (std::dynamic_pointer_cast<PhiIR>(user->defined_in_)) {
      auto phi = std::dynamic_pointer_cast<PhiIR>(user->defined_in_);
      TryRemoveTrivialPhi(phi);
    }
  }

}

auto Block::ReadVariable(const std::shared_ptr<VarSymbol> &var) -> std::shared_ptr<IRStatement> {
  /**
   * 这里ReadVariable得到的iter，有可能会读到一个已经消亡的var
   * 
   * */
  auto iter = var2def_.find(var);
  if (iter != var2def_.end()) {
    return iter->second;
  }
  std::shared_ptr<IRStatement> val;
  if (!is_sealed) {
    val = NewPhi(var);
    incomplete_phis_[var] = std::dynamic_pointer_cast<PhiIR>(val);
    WriteVariable(var, val);
  } else if (parent_.size() == 1) {
    val = parent_[0]->ReadVariable(var);
  } else {
    auto phi = NewPhi(var);
    AddPhiOperands(var, phi);
    val = phi;
  }
  WriteVariable(var, val);
  return val;
}

auto Block::WriteVariable(const std::shared_ptr<VarSymbol> &var, const std::shared_ptr<IRStatement> &stmt) -> void {
  var2def_[var] = stmt;
}

auto Block::IsSealed() const -> bool {
  return is_sealed;
}

auto Block::Seal() -> void {
  for (const auto &phi: incomplete_phis_) {
    auto var = phi.first;
    auto phi_stmt = phi.second;
    AddPhiOperands(var, phi_stmt);
  }
  is_sealed = true;
}

auto Block::IsGblInitBlock() const -> bool {
  return false;
}

auto Block::InsertIRs(const std::vector<std::shared_ptr<IRStatement>> &stmts) -> void {
  stmts_.insert(stmts_.end(), stmts.begin(), stmts.end());
}

auto Block::GetStmts() const -> const std::vector<std::shared_ptr<IRStatement>> & {
  return stmts_;
}

auto Block::GetBlockID() const -> size_t {
  return block_id_;
}

auto Block::SetSequenceID(size_t id) -> void {
  sequence_id_ = id;
}

auto Block::GetSequenceID() const -> size_t {
  return sequence_id_;
}

auto Block::AdjustID(size_t &ir_sequence_id) -> void {
  // 调整phi语句，把phi语句都放到block的最前面
  std::vector<std::shared_ptr<IRStatement>> adjusted_stmts;
  // 先把blockDeclear和phi放进来
  for (auto &stmt: stmts_) {
    if (std::dynamic_pointer_cast<BlockDeclIR>(stmt)) {
      // 设置block的sequence ID
      stmt->SetSequenceID(sequence_id_);
      ++sequence_id_;
      adjusted_stmts.push_back(stmt);
    } else if (std::dynamic_pointer_cast<PhiIR>(stmt)) {
      // 设置PhiIR的ID
      stmt->SetSequenceID(ir_sequence_id);
      ++ir_sequence_id;
      adjusted_stmts.push_back(stmt);
    }
  }
  for (auto &stmt: stmts_) {
    if (!std::dynamic_pointer_cast<BlockDeclIR>(stmt) && !std::dynamic_pointer_cast<PhiIR>(stmt)) {
      // 设置除了BlockDecelar, BRIR, ConstantIR, RreIR, StroeIR以外的sequence ID
      if (!std::dynamic_pointer_cast<BRIR>(stmt) && !std::dynamic_pointer_cast<ConstantIR>(stmt)
          && !std::dynamic_pointer_cast<RetIR>(stmt) && !std::dynamic_pointer_cast<StoreIR>(stmt)
          && !(std::dynamic_pointer_cast<CallIR>(stmt) && stmt->GetIRType() == IRTYPE::IR_VOID)) {
        stmt->SetSequenceID(ir_sequence_id);
        ++ir_sequence_id;

      }
      adjusted_stmts.push_back(stmt);
    }
    // if (!std::dynamic_pointer_cast<ConstantIR>(stmt)) {
    //   stmt->numbering = numbering++;
    // }
  }
  stmts_.swap(adjusted_stmts);
}

auto Block::SetDefUse() -> void {
  for (auto &stmt: stmts_) {
    stmt->SetDefUse();
  }
}

auto Block::AddReturnVal(std::shared_ptr<PhiIR> &phi, IRTYPE::IRTYPE ret_type) -> void {
  std::set < size_t > branchs;
  std::vector<std::shared_ptr<IRStatement>> oprnds = phi->GetOprnds();
  for (size_t i = 1; i < oprnds.size(); i += 2) {
    branchs.insert(oprnds[i]->GetID());
  }
  for (auto parent: parent_) {
    if (!branchs.count(parent->GetBlockID())) {
      // auto zero = std::make_shared<ConstantIR>(0, ret_type);
      auto zero = ConstantIR::Create(0, ret_type);
      phi->AddOprnd(zero);
      phi->AddOprnd(parent->GetStmts()[0]);
    }
  }
}

GblInitBlock::GblInitBlock() : Block(nullptr, nullptr, nullptr) {
}


auto GblInitBlock::IsGblInitBlock() const -> bool {
  return true;
}

auto GblInitBlock::ToSSA() const -> std::string {
  std::stringstream ssa;
  for (size_t i = 1; i < stmts_.size(); i++) {
    if (std::dynamic_pointer_cast<ConstantIR>(stmts_[i])) {
      // ConstantIR 不能输出
      continue;
    }
    ssa << stmts_[i]->ToSSA() << "\n";
  }
  return ssa.str();
}

auto GblInitBlock::SetDefUse() -> void {
  for (auto &stmt: stmts_) {
    stmt->SetDefUse();
  }
}

auto GblInitBlock::AdjustID() -> void {
  size_t block_id = 0;
  size_t ir_id = 0;
  for (auto &stmt: stmts_) {
    if (std::dynamic_pointer_cast<BlockDeclIR>(stmt)) {
      stmt->SetSequenceID(block_id);
      ++block_id;
    }
    if (!std::dynamic_pointer_cast<BlockDeclIR>(stmt) && !std::dynamic_pointer_cast<PhiIR>(stmt)) {
      // 设置除了BlockDecelar, BRIR, ConstantIR, RetIR, StroeIR以外的sequence ID, callIR
      if (!std::dynamic_pointer_cast<BRIR>(stmt) && !std::dynamic_pointer_cast<ConstantIR>(stmt)
          && !std::dynamic_pointer_cast<RetIR>(stmt) && !std::dynamic_pointer_cast<StoreIR>(stmt)
          && !(std::dynamic_pointer_cast<CallIR>(stmt) && stmt->GetIRType() == IRTYPE::IR_VOID)) {
        stmt->SetSequenceID(ir_id);
        ++ir_id;
      }
    }
  }
}

} // SCNUCC