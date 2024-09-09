#include "../include/AsmBlock.h"
#include "../include/AsmInstruction.h"
#include "../include/RegisterManager.h"
#include "../include/Location.h"

namespace SCNUCC {

auto ParallelCopy::AddCopy(std::shared_ptr<Location> &src, std::shared_ptr<Location> &dst) -> void {
  if (src == dst) {
    return;
  }
  copies_.insert(std::make_pair(src, dst));
}

auto ParallelCopy::FindNoCycleCopy() -> std::pair<std::shared_ptr<Location>, std::shared_ptr<Location>> {
  for (auto &copy: copies_) {
    // 拷贝的方向是copy.first -> copy.second
    bool flag = true;  // 标记这个
    for (auto &copy2: copies_) {
      if (copy == copy2) {
        continue;
      }
      if (copy.second == copy2.first) {
        // 对于a -> b，不存在b -> c，否则b的值会被a覆盖
        flag = false;
        break;
      }
    }
    if (flag) {
      return copy;
    }
  }
  return std::make_pair(nullptr, nullptr);
}

auto ParallelCopy::GenerateCopyInstr() -> AsmInstrPtrVec {
  AsmInstrPtrVec res;
  for (auto &copy: copies_) {
    if (*copy.first == *copy.second) {
      throw std::runtime_error("Copy src and dst are the same");
    }
  }

  while (!copies_.empty()) {
    auto target_copy = FindNoCycleCopy();
    if (target_copy.first) {
      Copy2Instr(target_copy.first, target_copy.second, res);
      copies_.erase(target_copy);
    } else {
      auto copy = *copies_.begin();
      auto new_var = reg_manager_->GetFreeStack();
      auto a = std::static_pointer_cast<Location>(new_var);
      Copy2Instr(a, copy.second, res);
      AddCopy(copy.first, a);
      copies_.erase(copy);
      reg_manager_->ReleaseStack(new_var);
    }
  }
  return res;
}

auto ParallelCopy::Copy2Instr(const std::shared_ptr<Location> &move_from, const std::shared_ptr<Location> &move_to,
                              AsmInstrPtrVec &res) -> void {
  if (move_to == move_from) {
    return;
  }
  if (move_to->type == Location::REGISTER && move_from->type == Location::REGISTER) {
    auto move_to_reg = std::dynamic_pointer_cast<Register>(move_to);
    auto move_from_reg = std::dynamic_pointer_cast<Register>(move_from);
    auto move_instr = std::make_shared<MoveInstruction>(move_to_reg, move_from_reg);
    move_instr->comment_ = "Copy2Instr: Remove phi";
    res.push_back(move_instr);
  } else if (move_to->type == Location::REGISTER &&
             move_from->type == Location::STACK) {
    auto move_to_reg = std::dynamic_pointer_cast<Register>(move_to);
    auto move_from_stack = std::dynamic_pointer_cast<Stack>(move_from);
    auto load_instr = std::make_shared<LoadInstruction>(move_to_reg, move_from_stack);
    load_instr->comment_ = "Copy2Instr: Remove phi";
    res.push_back(load_instr);
  } else if (move_to->type == Location::REGISTER &&
             (move_from->type == Location::IMMEDIATE12 || move_from->type == Location::IMMEDIATE20 ||
              move_from->type == Location::IMMEDIATE32)) {
    auto move_to_reg = std::dynamic_pointer_cast<Register>(move_to);
    auto move_from_imm = std::dynamic_pointer_cast<Immediate32>(move_from);
    if (!move_from_imm->is_int_) {  // 浮点数需要使用一个整数寄存器作为中转
      auto tmp_reg = reg_manager_->GetFreeIntReg();
      tmp_reg->new_data_home_ = nullptr;
      auto li_instr = std::make_shared<LoadImmInstruction>(tmp_reg, move_from_imm);
      li_instr->comment_ = "Copy2Instr: Load Imm to int register";
      res.push_back(li_instr);
      auto mitf_instr = std::make_shared<MoveIntToFloatInstruction>(move_to_reg, tmp_reg);
      mitf_instr->comment_ = "Copy2Instr: Move from int register to float register";
      res.push_back(mitf_instr);
      reg_manager_->ReleaseRegister(tmp_reg);
    } else {  // 整数直接li
      auto li_instr = std::make_shared<LoadImmInstruction>(move_to_reg, move_from_imm);
      li_instr->comment_ = "Copy2Instr: Load Imm to int Register";
      res.push_back(li_instr);
    }
  } else if (move_to->type == Location::STACK &&
             move_from->type == Location::REGISTER) {
    auto move_from_reg = std::dynamic_pointer_cast<Register>(move_from);
    auto move_to_stack = std::dynamic_pointer_cast<Stack>(move_to);
    auto st_instr = std::make_shared<StoreInstruction>(move_to_stack, move_from_reg);
    st_instr->comment_ = "Copy2Instr: Store register to stack";
    res.push_back(st_instr);
  } else if (move_to->type == Location::STACK &&
             move_from->type == Location::STACK) {
    auto move_to_stack = std::dynamic_pointer_cast<Stack>(move_to);
    auto move_from_stack = std::dynamic_pointer_cast<Stack>(move_from);
    auto tmp_reg = reg_manager_->GetFreeIntReg();
    tmp_reg->new_data_home_ = move_from_stack;
    auto load_instr = std::make_shared<LoadInstruction>(tmp_reg, move_from_stack);
    load_instr->comment_ = "Copy2Instr: Load stack data to temporary register";
    res.push_back(load_instr);
    auto store_instr = std::make_shared<StoreInstruction>(move_to_stack, tmp_reg);
    store_instr->comment_ = "Copy2Instr: The Store the register to stack";
    res.push_back(store_instr);
    reg_manager_->ReleaseRegister(tmp_reg);
  } else if (move_to->type == Location::STACK &&
             (move_from->type == Location::IMMEDIATE12 || move_from->type == Location::IMMEDIATE20 ||
              move_from->type == Location::IMMEDIATE32)) {
    auto move_to_stack = std::dynamic_pointer_cast<Stack>(move_to);
    auto move_from_imm = std::dynamic_pointer_cast<Immediate32>(move_from);
    auto tmp_reg = reg_manager_->GetFreeIntReg();
    tmp_reg->new_data_home_ = nullptr;
    auto li_instr = std::make_shared<LoadImmInstruction>(tmp_reg, move_from_imm);
    li_instr->comment_ = "Copy2Instr: Load imm to temporary register";
    res.push_back(li_instr);
    auto store_instr = std::make_shared<StoreInstruction>(move_to_stack, tmp_reg);
    store_instr->comment_ = "Copy2Instr: Store imm to stack then";
    res.push_back(store_instr);
    reg_manager_->ReleaseRegister(tmp_reg);
  } else {
    throw std::runtime_error("Unknown Location Type");
  }
}


std::string AsmBlock::ToString(const std::string &allocate, const std::string &destroy) {
  std::string res;
  if (block_flag_ == 1) {
    res += "\n\n    .text\n    .globl " + block_name_ + "\n";
  }
  res += block_name_ + ":\n";
  if (block_flag_ == 1) {
    res += "\t" + allocate + "\n";
  }
  for (auto &instr: instructions_) {
    if (block_flag_ == 2 && instr->type_ == AsmInstructionType::RET) {
      res += "\t" + destroy + "\n";
    }
    res += "\t" + instr->ToString() + "\n";
  }
  for (auto &instr: parallel_copy_->GenerateCopyInstr()) {
    res += "\t" + instr->ToString() + "\n";
  }

  if (GetChildNum() == 2) {
    res += "\t bnez " + reg2str[jump_cond_] + ", " + true_br_->block_name_ + "\n";
    res += "\t j " + false_br_->block_name_ + "\n";
  } else if (GetChildNum() == 1) {
    res += "\t j " + true_br_->block_name_ + "\n";
  }
  return res;
}

auto AsmBlock::SetJumpCond(SCNUCC::RegisterName cond) -> void {
  jump_cond_ = cond;
}

auto AsmBlock::SetTrueBranch(const SCNUCC::AsmBlock::AsmBlockPtr &true_br) -> void {
  true_br_ = true_br;
}

auto AsmBlock::SetFalseBranch(const SCNUCC::AsmBlock::AsmBlockPtr &false_br) -> void {
  false_br_ = false_br;
}

auto AsmBlock::ChangeBranch(const SCNUCC::AsmBlock::AsmBlockPtr &old, const SCNUCC::AsmBlock::AsmBlockPtr &neww) -> void {
  if (old == true_br_) {
    true_br_ = neww;
  } else if(old == false_br_) {
    false_br_ = neww;
  } else {
    throw "old branch not found";
  }

}

} // namespace SCNUCC