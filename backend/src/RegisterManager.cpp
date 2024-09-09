#include <assert.h>
#include <algorithm>
#include <cmath>
#include "../include/RegisterManager.h"
#include "../include/Location.h"
#include "../include/riscv.h"
#include "../../frontend/include/IRVariable.h"
#include "../../frontend/include/IRStatement.h"
#include "../include/AsmInstruction.h"
#include "../include/AsmBlock.h"

namespace SCNUCC {

auto MyQueue::push(const RegisterPtr &reg) -> void {
  regs_.insert(reg);
}

auto MyQueue::pop() -> void {
  regs_.erase(regs_.begin());
}

auto MyQueue::top() -> RegisterPtr {
  return *regs_.begin();
}

auto MyQueue::empty() -> bool {
  return regs_.empty();
}

auto MyQueue::erase(const SCNUCC::RegisterPtr &reg) -> void {
  regs_.erase(reg);
}


bool MyQueue::RegisterCmp::operator()(const RegisterPtr &a, const RegisterPtr &b) const {
  // 返回 true 表示 a 的优先级低于 b
  // TODO: 排序规则
  // 1. 优先使用t类型寄存器，然后是s类型寄存器，最后是a类型寄存器
  // 2. 如果上面的条件还不能判断，则优先使用编号小的寄存器
  if (a->name_[0] != b->name_[0]) {  // 如果寄存器的类别不同，比如一个是a类，一个是t类
    return a->name_[0] < b->name_[0];
  }
  return a->name_ > b->name_;
}

bool RegisterManager::StackCmp::operator()(StackPtr &a, StackPtr &b) {
  if (!a) {
    throw std::runtime_error("Null pointer encountered in StackCmp::operator(A)");
  }
  if (!b) {
    throw std::runtime_error("Null pointer encountered in StackCmp::operator(B)");
  }
  return a->offset_ > b->offset_;
}


RegisterManager::RegisterManager(std::map<IRVariablePtr, std::shared_ptr<Location>> &var2location,
                                 std::shared_ptr<AsmBlock> &cur_blk)
        : var2location_(var2location), cur_blk_(cur_blk) {
  var2location_ = var2location;

  // 先记录那些寄存器已经被分配了
  std::set<RegisterName> used_regs;
  for (auto &var: var2location) {
    if (var.second->type == Location::REGISTER) {
      auto reg = std::dynamic_pointer_cast<Register>(var.second);
      used_regs.insert(reg->reg_);
      name2ptr_[reg->reg_] = reg;
    }
  }

  // 遍历所有的整数寄存器，将空闲的寄存器放入empty_int_regs_，将被占用的寄存器放入evictable_int_regs_
  for (auto reg_name = RegisterName::T0; reg_name <= RegisterName::A7;
       reg_name = static_cast<RegisterName>(reg_name + 1)) {
    if (reg_name == RegisterName::S0) {
      continue;
    }
    if (used_regs.find(reg_name) == used_regs.end()) {
      auto reg = std::shared_ptr<Register>(new Register(reg_name));
      if (reg_name != RegisterName::S0) {
        empty_int_regs_.push(reg);
      }
      name2ptr_[reg_name] = reg;
    } else {
      evictable_int_regs_.push(name2ptr_[reg_name]);
      if (reg_name <= RegisterName::T6) {  // reg <= T6
        used_t_regs_.insert(reg_name);
      } else if (reg_name <= RegisterName::S11) {  // reg <= S11
        used_s_regs_.insert(reg_name);
        auto stk = GetFreeStack();
        cur_blk_->AddInstruction(std::make_shared<StoreInstruction>(stk, name2ptr_[reg_name]));
        cur_blk_->instructions_.back()->comment_ = "RegisterManager: Get a callee-saved register, need to save it to the stack";
        callee_saved_to_[reg_name] = stk;
      } else {  // reg <= A7
        used_a_regs_.insert(reg_name);
      }
    }
  }

  // 遍历所有的浮点寄存器，将空闲的寄存器放入empty_float_regs_，将被占用的寄存器放入evictable_float_regs_
  for (auto reg_name = RegisterName::FT0; reg_name <= RegisterName::FA7;
       reg_name = static_cast<RegisterName>(reg_name + 1)) {
    if (used_regs.find(reg_name) == used_regs.end()) {
      auto reg = std::shared_ptr<Register>(new Register(reg_name));
      empty_float_regs_.push(reg);
      name2ptr_[reg_name] = reg;
    } else {
      evictable_float_regs_.push(name2ptr_[reg_name]);
      if (reg_name <= RegisterName::FT6) {  // reg <= FT6
        used_ft_regs_.insert(reg_name);
      } else if (reg_name <= RegisterName::FS11) {  // reg <= FS11
        used_fs_regs_.insert(reg_name);
        auto stk = GetFreeStack();
        cur_blk_->AddInstruction(std::make_shared<StoreInstruction>(stk, name2ptr_[reg_name]));
        cur_blk_->instructions_.back()->comment_ = "RegisterManager: Get a callee-saved register, need to save it to the stack";
        callee_saved_to_[reg_name] = stk;
      } else {  // reg <= FA7
        used_fa_regs_.insert(reg_name);
      }
    }
  }

  // 生成一些特殊的寄存器
  name2ptr_[RegisterName::ZERO] = std::shared_ptr<Register>(new Register(RegisterName::ZERO));
  name2ptr_[RegisterName::RA] = std::shared_ptr<Register>(new Register(RegisterName::RA));
  name2ptr_[RegisterName::SP] = std::shared_ptr<Register>(new Register(RegisterName::SP));
  name2ptr_[RegisterName::GP] = std::shared_ptr<Register>(new Register(RegisterName::GP));
  name2ptr_[RegisterName::TP] = std::shared_ptr<Register>(new Register(RegisterName::TP));
  name2ptr_[RegisterName::S0] = std::shared_ptr<Register>(new Register(RegisterName::S0));
}

auto RegisterManager::SetCurBlock(std::shared_ptr<AsmBlock> &blk) -> void {
  cur_blk_ = blk;
}

auto RegisterManager::AssignRegister(std::vector<IRVariablePtr> &vars) -> std::vector<RegisterPtr> {
  std::vector<RegisterPtr> regs;
  // 如果一个变量已经占用了一个寄存器，那么就直接使用这个寄存器
  for (auto &var: vars) {
    if (var->home_location_->type == Location::REGISTER) {
      auto reg = std::dynamic_pointer_cast<Register>(var->home_location_);
      if (var->type_ == IRVariable::Type::INT || var->type_ == IRVariable::Type::INT_POINTER ||
          var->type_ == IRVariable::Type::FLOAT_POINTER || var->type_ == IRVariable::Type::BOOL) {
        // var占用了一个整数寄存器，那么这个寄存器就不能被evict了
        evictable_int_regs_.erase(reg);
      } else {
        evictable_float_regs_.erase(reg);
      }
    }
  }

  // 优先使用已经在寄存器中的变量
  for (auto &var: vars) {
    if (var->home_location_->type == Location::REGISTER) {
      // 如果var已经在寄存器中，那么就直接使用这个寄存器
      auto reg = std::dynamic_pointer_cast<Register>(var->home_location_);
      reg->no_need_to_load_store = true;  // 因为数据本身就在这个寄存器里面，所以这个寄存器在释放时不需要load或者store
      regs.push_back(reg);
    } else if (var->home_location_->type == Location::STACK) {  // 保证var是一个被分配了空间的变量，而不是label，imm这些
      // 如果var在内存中，就为var分配一个寄存器
      RegisterPtr reg;
      int load_float_ = 0;
      if (var->type_ == IRVariable::Type::INT || var->type_ == IRVariable::Type::INT_POINTER
          || var->type_ == IRVariable::Type::FLOAT_POINTER || var->type_ == IRVariable::Type::BOOL) {
        reg = GetFreeIntReg();
      } else {
        reg = GetFreeFloatReg();
        load_float_ = 1;
      }
      reg->new_data_home_ = std::dynamic_pointer_cast<Stack>(var->home_location_);
      cur_blk_->AddInstruction(std::make_shared<LoadInstruction>(reg, reg->new_data_home_));
      cur_blk_->instructions_.back()->comment_ = "AssignRegister: Load variable from stack to register";
      regs.push_back(reg);
    } else if (var->home_location_->type == Location::HEAP) {
      // 在堆中，说明这是一个全局变量，那么就申请一个寄存器，然后把这个变量在堆中的地址加载到寄存器中
      RegisterPtr reg = GetFreeIntReg();
      reg->new_data_home_ = nullptr;
      auto label = std::dynamic_pointer_cast<Heap>(var->home_location_)->name_;
      cur_blk_->AddInstruction(std::make_shared<LoadAddrInstruction>(reg, label));
      cur_blk_->instructions_.back()->comment_ = "AssignRegister: Load address of global variable to register";
      regs.push_back(reg);
    } else {
      // 如果是立即数，那么使用li指令将立即数加载到寄存器中
      auto reg = GetFreeIntReg();
      reg->new_data_home_ = nullptr;
      auto imm = std::dynamic_pointer_cast<Immediate32>(var->home_location_);
      cur_blk_->AddInstruction(std::make_shared<LoadImmInstruction>(reg, imm));
      cur_blk_->instructions_.back()->comment_ = "AssignRegister: Load Imm32 to register";
      regs.push_back(reg);

      if (var->type_ == IRVariable::Type::FLOAT || var->type_ == IRVariable::Type::FLOAT_IMM) {
        // 如果是浮点数，那么需要将整数寄存器中的数据移动到浮点寄存器中
        auto float_reg = GetFreeFloatReg();
        float_reg->new_data_home_ = nullptr;
        cur_blk_->AddInstruction(std::make_shared<MoveIntToFloatInstruction>(float_reg, reg));
        cur_blk_->instructions_.back()->comment_ = "AssignRegister: The variable is float, move to float register";
        ReleaseRegister(reg);  // 原本的整数寄存器已经没用了，所以释放掉
        regs.pop_back();
        regs.push_back(float_reg);
      }
    }
  }

  return regs;
}

auto RegisterManager::GetFreeIntReg() -> RegisterPtr {
  if (!empty_int_regs_.empty()) {  // 优先使用空的寄存器
    auto reg = empty_int_regs_.top();
    empty_int_regs_.pop();
    if (reg->reg_ <= RegisterName::T6) {  // reg <= T6
      used_t_regs_.insert(reg->reg_);
    } else if (reg->reg_ <= RegisterName::S11) {  // reg <= S11
      used_s_regs_.insert(reg->reg_);
      if (callee_saved_to_.find(reg->reg_) == callee_saved_to_.end()) {
        // 如果是s1到s11，那么需要将这个寄存器的数据store到栈中；在return之前再load回来
        auto stk = GetFreeStack();
        cur_blk_->AddInstruction(std::make_shared<StoreInstruction>(stk, reg));
        cur_blk_->instructions_.back()->comment_ = "GetFreeIntReg: Get a callee-saved register, need to save it to the stack";
        callee_saved_to_[reg->reg_] = stk;
      }
    } else {  // reg <= A7
      used_a_regs_.insert(reg->reg_);
    }
    reg->old_data_stk_ = nullptr;
    return reg;
  }
  // 如果没有空闲寄存器，那么就选择一个被evict的寄存器
  auto reg = evictable_int_regs_.top();
  evictable_int_regs_.pop();
  auto stk = GetFreeStack();  // 选择一个栈空间用来存储这个寄存器的数据
  cur_blk_->AddInstruction(std::make_shared<StoreInstruction>(stk, reg));
  cur_blk_->instructions_.back()->comment_ = "GetFreeIntReg: Store the original data of register to the stack";
  reg->old_data_stk_ = stk;

  return reg;
}

auto RegisterManager::GetFreeFloatReg() -> RegisterPtr {
  if (!empty_float_regs_.empty()) {  // 优先使用空的寄存器
    auto reg = empty_float_regs_.top();
    empty_float_regs_.pop();
    if (reg->reg_ <= RegisterName::FT6) {  // reg <= FT6
      used_ft_regs_.insert(reg->reg_);
    } else if (reg->reg_ <= RegisterName::FS11) {  // reg <= FS11
      used_fs_regs_.insert(reg->reg_);
      if (callee_saved_to_.find(reg->reg_) == callee_saved_to_.end()) {
        // 如果是s1到s11，那么需要将这个寄存器的数据store到栈中；在return之前再load回来
        auto stk = GetFreeStack();
        cur_blk_->AddInstruction(std::make_shared<StoreInstruction>(stk, reg));
        cur_blk_->instructions_.back()->comment_ = "GetFreeIntReg: Get a callee-saved register, need to save it to the stack";
        callee_saved_to_[reg->reg_] = stk;
      }
    } else {  // reg <= FA7
      used_fa_regs_.insert(reg->reg_);
    }
    reg->old_data_stk_ = nullptr;
    return reg;
  }
  auto reg = evictable_float_regs_.top();
  evictable_float_regs_.pop();
  auto stk = GetFreeStack();
  cur_blk_->AddInstruction(std::make_shared<StoreInstruction>(stk, reg));
  cur_blk_->instructions_.back()->comment_ = "GetFreeFloatReg: Store the original data of register to the stack";
  reg->old_data_stk_ = stk;

  return reg;
}

auto RegisterManager::ReleaseRegister(const SCNUCC::RegisterPtr &reg) -> void {
  if (reg->no_need_to_load_store) {  // 这个寄存器在linear scan阶段就已经被分配出去
    reg->no_need_to_load_store = false;
    if (reg->reg_ <= RegisterName::A7) {
      evictable_int_regs_.push(reg);
    } else {
      evictable_float_regs_.push(reg);
    }
    return; // TODO: 这个逻辑有问题：假如t0寄存器在linear scan阶段就被分配出去了，然后t0被其他变量临时占用，那么逻辑就有问题
  }
  if (!reg->new_data_home_) {
    // 如果new_data_home_为空，那就不需要store回去
    if (!reg->old_data_stk_) {
      // 如果old_data_stk_也为空，那么说明这个寄存器本来就是空的，直接放回空闲寄存器队列
      if (reg->reg_ <= RegisterName::A7) {
        empty_int_regs_.push(reg);
      } else {
        empty_float_regs_.push(reg);
      }
    } else {
      // 如果old_data_stk_不为空，那么说明这个寄存器之前的变量现在被存储在栈中，所以需要load回来
      cur_blk_->AddInstruction(std::make_shared<LoadInstruction>(reg, reg->old_data_stk_));
      cur_blk_->instructions_.back()->comment_ = "ReleaseRegister: Load the original data from stack";
      ReleaseStack(reg->old_data_stk_);
      if (reg->reg_ <= RegisterName::A7) {
        evictable_int_regs_.push(reg);
      } else {
        evictable_float_regs_.push(reg);
      }
    }
  } else { // 如果new_data_home_不为空，那么说明这个寄存器中的数据是一个变量
    // 否则，把暂时占用寄存器的数据store回原来的位置
    cur_blk_->AddInstruction(std::make_shared<StoreInstruction>(reg->new_data_home_, reg));
    cur_blk_->instructions_.back()->comment_ = "ReleaseRegister: Store the new data back to stack";
    if (reg->old_data_stk_) {
      // old_data_stk_不为nullptr，表明这个寄存器之前的变量现在被存储在栈中，所以需要load回来
      cur_blk_->AddInstruction(std::make_shared<LoadInstruction>(reg, reg->old_data_stk_));
      cur_blk_->instructions_.back()->comment_ = "ReleaseRegister: Load the original data from stack";
      ReleaseStack(reg->old_data_stk_);
      if (reg->reg_ <= RegisterName::A7) {
        evictable_int_regs_.push(reg);
      } else {
        evictable_float_regs_.push(reg);
      }
    } else {
      // 如果old_data_stk_为nullptr，表明这个寄存器是空的，所以直接放回空闲寄存器队列
      if (reg->reg_ <= RegisterName::A7) {
        empty_int_regs_.push(reg);
      } else {
        empty_float_regs_.push(reg);
      }
    }
  }

  reg->new_data_home_ = nullptr;
  reg->old_data_stk_ = nullptr;
  if (reg->reg_ <= RegisterName::T6) {  // reg <= T6
    used_t_regs_.erase(reg->reg_);
  } else if (reg->reg_ <= RegisterName::S11) {  // reg <= S11
    used_s_regs_.erase(reg->reg_);
  } else if (reg->reg_ <= RegisterName::A7) {  // reg <= A7
    used_a_regs_.erase(reg->reg_);
  } else if (reg->reg_ <= RegisterName::FT6) {  // reg <= FT6
    used_ft_regs_.erase(reg->reg_);
  } else if (reg->reg_ <= RegisterName::FS11) {  // reg <= FS11
    used_fs_regs_.erase(reg->reg_);
  } else {  // reg <= FA7
    used_fa_regs_.erase(reg->reg_);
  }
}

auto RegisterManager::GetFreeStack() -> StackPtr {
  if (!available_stk_.empty()) {
    auto stk = available_stk_.top();
    available_stk_.pop();
    return stk;
  }
  return NewStackLocation();
}

auto RegisterManager::ReleaseRegister(const std::vector<RegisterPtr> &reg) -> void {
  for (auto &r: reg) {
    ReleaseRegister(r);
  }
}

auto RegisterManager::ReleaseStack(const StackPtr &stk) -> void {
  auto comment = std::make_shared<Comment>("# Release Stack " + stk->name_);
  cur_blk_->AddInstruction(comment);
  available_stk_.push(stk);
}

auto RegisterManager::NewStackLocation() -> std::shared_ptr<Stack> {
//  auto stk = std::make_shared<Stack>(stk_size_);
  auto stk = std::shared_ptr<Stack>(new Stack());
  auto comment = std::make_shared<Comment>("# NewStackLocation: New a Stack " + stk->name_);
  cur_blk_->AddInstruction(comment);
  return stk;
}

auto RegisterManager::GetSpecRegister(RegisterName name) -> RegisterPtr {
  return name2ptr_[name];
}

auto RegisterManager::CallerSaveRegisters(std::shared_ptr<CallIR> &call_ir) -> void {
  caller_saved_to_.clear();
  auto var = call_ir->GetResultVar();
  for (auto &reg: used_t_regs_) {
    if (var->type_ != IRVariable::Type::VOID) { // void函数
      if (var->home_location_->type == Location::REGISTER
          && std::dynamic_pointer_cast<Register>(var->home_location_)->reg_ == reg) {
        continue;
      }
    }
    auto stk = GetFreeStack();
    cur_blk_->AddInstruction(std::make_shared<StoreInstruction>(stk, name2ptr_[reg]));
    cur_blk_->instructions_.back()->comment_ = "CallerSaveRegisters: Save t type registers to stack";
    caller_saved_to_[reg] = stk;
  }
  for (auto &reg: used_a_regs_) {
    if (var->type_ != IRVariable::Type::VOID) { // void函数
      // 非void函数不需要保存返回值的那个寄存器
      if (var->home_location_->type == Location::REGISTER
          && std::dynamic_pointer_cast<Register>(var->home_location_)->reg_ == reg) {
        continue;
      }
    }
    auto stk = GetFreeStack();
    cur_blk_->AddInstruction(std::make_shared<StoreInstruction>(stk, name2ptr_[reg]));
    cur_blk_->instructions_.back()->comment_ = "CallerSaveRegisters: Save a type registers to stack";
    caller_saved_to_[reg] = stk;
  }
  for (auto &reg: used_ft_regs_) {
    if (var->type_ != IRVariable::Type::VOID) { // void函数
      // 非void函数不需要保存返回值的那个寄存器
      if (var->home_location_->type == Location::REGISTER
          && std::dynamic_pointer_cast<Register>(var->home_location_)->reg_ == reg) {
        continue;
      }
    }
    auto stk = GetFreeStack();
    cur_blk_->AddInstruction(std::make_shared<StoreInstruction>(stk, name2ptr_[reg]));
    cur_blk_->instructions_.back()->comment_ = "CallerSaveRegisters: Save ft type register to stack";
    caller_saved_to_[reg] = stk;
  }
  for (auto &reg: used_fa_regs_) {
    if (var->type_ != IRVariable::Type::VOID) { // void函数
      if (var->home_location_->type == Location::REGISTER
          && std::dynamic_pointer_cast<Register>(var->home_location_)->reg_ == reg) {
        continue;
      }
    }
    auto stk = GetFreeStack();
    cur_blk_->AddInstruction(std::make_shared<StoreInstruction>(stk, name2ptr_[reg]));
    cur_blk_->instructions_.back()->comment_ = "CallerSaveRegisters: Save fa type register to stack";
    caller_saved_to_[reg] = stk;
  }
}

auto RegisterManager::CallerLoadRegisters(std::shared_ptr<CallIR> &call_ir) -> void {
  auto var = call_ir->GetResultVar();
  for (auto &reg: used_t_regs_) {
    if (var->type_ != IRVariable::Type::VOID) { // void函数
      if (var->home_location_->type == Location::REGISTER
          && std::dynamic_pointer_cast<Register>(var->home_location_)->reg_ == reg) {
        continue;
      }
    }
    cur_blk_->AddInstruction(std::make_shared<LoadInstruction>(name2ptr_[reg], caller_saved_to_[reg]));
    cur_blk_->instructions_.back()->comment_ = "CallerLoadRegisters: Load t type register from stack";
    if (caller_saved_to_[reg] == nullptr) {
      throw std::runtime_error("CallerLoadRegisters: caller_saved_to_[reg] == nullptr");
    }
    ReleaseStack(caller_saved_to_[reg]);
  }
  for (auto &reg: used_a_regs_) {
    if (var->type_ != IRVariable::Type::VOID) { // void函数
      if (var->home_location_->type == Location::REGISTER
          && std::dynamic_pointer_cast<Register>(var->home_location_)->reg_ == reg) {
        continue;
      }
    }
    cur_blk_->AddInstruction(std::make_shared<LoadInstruction>(name2ptr_[reg], caller_saved_to_[reg]));
    cur_blk_->instructions_.back()->comment_ = "CallerLoadRegisters: Load a type register from stack";
    if (caller_saved_to_[reg] == nullptr) {
      throw std::runtime_error("CallerLoadRegisters: caller_saved_to_[reg] == nullptr");
    }
    ReleaseStack(caller_saved_to_[reg]);
  }
  for (auto &reg: used_ft_regs_) {
    if (var->type_ != IRVariable::Type::VOID) { // void函数
      if (var->home_location_->type == Location::REGISTER
          && std::dynamic_pointer_cast<Register>(var->home_location_)->reg_ == reg) {
        continue;
      }
    }
    cur_blk_->AddInstruction(std::make_shared<LoadInstruction>(name2ptr_[reg], caller_saved_to_[reg]));
    cur_blk_->instructions_.back()->comment_ = "CallerLoadRegisters: Load ft type register from stack";
    if (caller_saved_to_[reg] == nullptr) {
      throw std::runtime_error("CallerLoadRegisters: caller_saved_to_[reg] == nullptr");
    }
    ReleaseStack(caller_saved_to_[reg]);
  }
  for (auto &reg: used_fa_regs_) {
    if (var->type_ != IRVariable::Type::VOID) { // void函数
      if (var->home_location_->type == Location::REGISTER
          && std::dynamic_pointer_cast<Register>(var->home_location_)->reg_ == reg) {
        continue;
      }
    }
    cur_blk_->AddInstruction(std::make_shared<LoadInstruction>(name2ptr_[reg], caller_saved_to_[reg]));
    cur_blk_->instructions_.back()->comment_ = "CallerLoadRegisters: Load fa type register from stack";
    if (caller_saved_to_[reg] == nullptr) {
      throw std::runtime_error("CallerLoadRegisters: caller_saved_to_[reg] == nullptr");
    }
    ReleaseStack(caller_saved_to_[reg]);
  }
}

auto RegisterManager::CalleeLoadRegisters() -> void {
  for (auto &reg: used_s_regs_) {
    cur_blk_->AddInstruction(std::make_shared<LoadInstruction>(name2ptr_[reg], callee_saved_to_[reg]));
    cur_blk_->instructions_.back()->comment_ = "CalleeLoadRegisters: Load s type register from stack";
    if (callee_saved_to_[reg] == nullptr) {
      throw std::runtime_error("CalleeLoadRegisters: callee_saved_to_[reg] == nullptr");
    }
    ReleaseStack(callee_saved_to_[reg]);
  }
  for (auto &reg: used_fs_regs_) {
    cur_blk_->AddInstruction(std::make_shared<LoadInstruction>(name2ptr_[reg], callee_saved_to_[reg]));
    cur_blk_->instructions_.back()->comment_ = "CalleeLoadRegisters: Load fs type register from stack";
    if (callee_saved_to_[reg] == nullptr) {
      throw std::runtime_error("CalleeLoadRegisters: callee_saved_to_[freg] == nullptr");
    }
    ReleaseStack(callee_saved_to_[reg]);
  }
}

auto RegisterManager::SolveFuncCalling(std::shared_ptr<CallIR> &call_ir) -> void {
  // 1. 使用store指令把所有的函数参数都放入到栈空间
  // 2. 保存caller saved寄存器
  // 3. 使用load指令把栈中的函数参数加载到a0-a7寄存器中
  // 4. 把超过8个的寄存器放入到栈帧底部
  auto args = call_ir->GetOperands();
  max_args_size_ = std::max(max_args_size_, args.size());
  auto var2stk = std::vector<StackPtr>(args.size());

  // swap_reg用于中转站立即数
  auto swap_reg = name2ptr_[RegisterName::T6];
  auto swap_stk = GetFreeStack();
  cur_blk_->AddInstruction(std::make_shared<StoreInstruction>(swap_stk, swap_reg));
  cur_blk_->instructions_.back()->comment_ = "SolveFuncCalling: Save the register to stack, use the register as a swap area to store imm to the stack";
  // 现将所有的参数保存到栈上
  for (size_t i = 0; i < args.size(); i++) {
    auto arg = args[i];
    if (arg->home_location_->type == Location::REGISTER) {
      auto reg = std::dynamic_pointer_cast<Register>(arg->home_location_);
      auto stk = GetFreeStack();
      if (reg == swap_reg) {
        cur_blk_->AddInstruction(std::make_shared<LoadInstruction>(swap_reg, swap_stk));
        cur_blk_->instructions_.back()->comment_ = "SolveFuncCalling: The arg to be saved is the original data in the T6, then load the original data back to T6";
      }
      cur_blk_->AddInstruction(std::make_shared<StoreInstruction>(stk, reg));
      cur_blk_->instructions_.back()->comment_ = "SolveFuncCalling: The arg data to the stack.";
      var2stk[i] = stk;
    } else if (arg->home_location_->type == Location::STACK) {
      var2stk[i] = std::dynamic_pointer_cast<Stack>(arg->home_location_);
      cur_blk_->AddInstruction(std::make_shared<Comment>(
              "SolveFuncCalling: The the " + std::to_string(i) + "th args has been in stack " + var2stk[i]->name_ +
              ", not need to store a again."));
    } else if (arg->home_location_->type == Location::IMMEDIATE12 ||
               arg->home_location_->type == Location::IMMEDIATE20 ||
               arg->home_location_->type == Location::IMMEDIATE32) {
      auto imm = std::dynamic_pointer_cast<Immediate32>(arg->home_location_);
      auto stk = GetFreeStack();
      cur_blk_->AddInstruction(std::make_shared<LoadImmInstruction>(swap_reg, imm));
      cur_blk_->instructions_.back()->comment_ = "SolveFuncCalling: Load the imm to the swap area";
      cur_blk_->AddInstruction(std::make_shared<StoreInstruction>(stk, swap_reg));
      cur_blk_->instructions_.back()->comment_ = "SolveFuncCalling: Store the imm to stack then";
      var2stk[i] = stk;
    }
  }
  cur_blk_->AddInstruction(std::make_shared<LoadInstruction>(swap_reg, swap_stk));
  cur_blk_->instructions_.back()->comment_ = "SolveFuncCalling: Recover the swap area";
  if (swap_stk == nullptr) {
    throw std::runtime_error("SolveFuncCalling: swap_stk == nullptr");
  }
  ReleaseStack(swap_stk);
  // std::vector<std::string> 
  CallerSaveRegisters(call_ir);  // 保存t和a类寄存器

  // 将参数放入到指定的寄存器或者栈空间
  int int_param_num = 0, float_param_num = 0;
  for (size_t i = 0; i < args.size() && i < 8; i++) {
    auto arg = args[i];
    auto stk = var2stk[i];
    RegisterName reg_name;
    if (arg->type_ == IRVariable::Type::FLOAT || arg->type_ == IRVariable::Type::FLOAT_IMM) {
      reg_name = static_cast<RegisterName>(RegisterName::FA0 + i); // float
      ++float_param_num;
    } else {
      reg_name = static_cast<RegisterName>(RegisterName::A0 + i);
      ++int_param_num;
    }
    cur_blk_->AddInstruction(std::make_shared<LoadInstruction>(name2ptr_[reg_name], stk));
    cur_blk_->instructions_.back()->comment_ = "SolveFuncCalling: Load the " + std::to_string(i) + "th arg to register";
    if (stk == nullptr) {
      throw std::runtime_error("SolveFuncCalling: stk == nullptr");
    }
    ReleaseStack(stk);
  }
  for (size_t i = 8; i < args.size(); i++) {
    auto arg = args[i];
    auto stk = var2stk[i];
    cur_blk_->AddInstruction(std::make_shared<LoadInstruction>(name2ptr_[RegisterName::T0], stk));
    cur_blk_->instructions_.back()->comment_ =
            "SolveFuncCalling: Load the " + std::to_string(i) + "th arg to temporary register";
    auto arg_stk = std::shared_ptr<ArgsStack>(new ArgsStack((i - 8) * 8));
    cur_blk_->AddInstruction(std::make_shared<StoreArgsInstruction>(arg_stk, name2ptr_[RegisterName::T0]));
    cur_blk_->instructions_.back()->comment_ =
            "SolveFuncCalling: Store the the" + std::to_string(i) + "th arg to stack";
    if (stk == nullptr) {
      throw std::runtime_error("SolveFuncCalling: swap_stk == nullptr");
    }
    ReleaseStack(stk);
  }

  // 生成call instruction
  cur_blk_->AddInstruction(std::make_shared<CallInstruction>(call_ir->GetFuncName(), call_ir->GetParamsType()));
  if (call_ir->GetIRType() != IRTYPE::IR_VOID) {
    auto var = call_ir->GetResultVar();
    std::vector<IRVariablePtr> vec = {var};
    auto reg = AssignRegister(vec);
    if (reg[0]->reg_ >= RegisterName::T0 && reg[0]->reg_ <= RegisterName::A7) {
      cur_blk_->AddInstruction(std::make_shared<MoveInstruction>(reg[0], name2ptr_[RegisterName::A0]));
    } else {
      cur_blk_->AddInstruction(std::make_shared<MoveInstruction>(reg[0], name2ptr_[RegisterName::FA0]));
    }
    ReleaseRegister(reg);
  }
  cur_blk_->instructions_.back()->comment_ = "SolveFuncCalling: Call the function";
  // 恢复现场
  CallerLoadRegisters(call_ir);
}

auto RegisterManager::AlignSPWith16Byte() -> void {
  Stack::stack_size_ += (max_args_size_ > 8 ? max_args_size_ - 8 : 0) * 8;  // 加上函数参数部分
  auto rem = Stack::stack_size_ % 16;
  if (rem == 0) {
    return;
  }
  Stack::stack_size_ = Stack::stack_size_ + (16 - rem);
}

auto RegisterManager::SolveMemset(std::shared_ptr<ArrayDeclIR> &array_decl) -> void {
  // swap_reg用于中转站立即数
  auto swap_reg = name2ptr_[RegisterName::T6];
  auto swap_stk = GetFreeStack();
  cur_blk_->AddInstruction(std::make_shared<StoreInstruction>(swap_stk, swap_reg));
  cur_blk_->instructions_.back()->comment_ = "SolveFuncCalling: Save the register to stack, use the register as a swap area to store imm to the stack";

  auto var = array_decl->GetResultVar();
  int offset = std::dynamic_pointer_cast<Stack>(var->array_location_)->offset_;
  auto imm12 = std::make_shared<Immediate12>(offset, 1);
  // 现在的T6是可以用的，那么我需要把数组的首地址放到T6
  auto header = std::make_shared<BinaryImmInstruction>(AsmInstructionType::ADDI, swap_reg, name2ptr_[RegisterName::S0],
                                                       imm12);
  cur_blk_->AddInstruction(header);
  // 现在T6是首地址
  std::vector<StackPtr> var2stk;
  auto stk_for_header = GetFreeStack();
  var2stk.push_back(stk_for_header);
  auto store_header = std::make_shared<StoreInstruction>(stk_for_header, swap_reg);
  cur_blk_->AddInstruction(store_header);
  // 再保存一个0
  auto zero = std::make_shared<Immediate32>(0, 1);
  // 放到T6
  auto li_zero = std::make_shared<LoadImmInstruction>(swap_reg, zero);
  cur_blk_->AddInstruction(li_zero);
  // 再保存到栈上
  auto stk_for_zero = GetFreeStack();
  var2stk.push_back(stk_for_zero);
  auto store_zero = std::make_shared<StoreInstruction>(stk_for_zero, swap_reg);
  cur_blk_->AddInstruction(store_zero);
  // 获取长度
  int size = array_decl->GetArraySize() * 2;  // 改成八字节了
  auto size_imm = std::make_shared<Immediate32>(size, 1);
  auto li_size = std::make_shared<LoadImmInstruction>(swap_reg, size_imm);
  cur_blk_->AddInstruction(li_size);
  // 再保存到栈上
  auto stk_for_size = GetFreeStack();
  var2stk.push_back(stk_for_size);
  auto store_zero2 = std::make_shared<StoreInstruction>(stk_for_size, swap_reg);
  cur_blk_->AddInstruction(store_zero2);

  cur_blk_->AddInstruction(std::make_shared<LoadInstruction>(swap_reg, swap_stk));
  cur_blk_->instructions_.back()->comment_ = "SolveFuncCalling: Recover the swap area";
  if (swap_stk == nullptr) {
    throw std::runtime_error("SolveFuncCalling: swap_stk == nullptr");
  }
  ReleaseStack(swap_stk);

  // caller_save
  caller_saved_to_.clear();
  for (auto &reg: used_t_regs_) {
    auto stk = GetFreeStack();
    cur_blk_->AddInstruction(std::make_shared<StoreInstruction>(stk, name2ptr_[reg]));
    cur_blk_->instructions_.back()->comment_ = "CallerSaveRegisters: Save t type registers to stack";
    caller_saved_to_[reg] = stk;
  }
  for (auto &reg: used_a_regs_) {
    auto stk = GetFreeStack();
    cur_blk_->AddInstruction(std::make_shared<StoreInstruction>(stk, name2ptr_[reg]));
    cur_blk_->instructions_.back()->comment_ = "CallerSaveRegisters: Save a type registers to stack";
    caller_saved_to_[reg] = stk;
  }
  for (auto &reg: used_ft_regs_) {
    auto stk = GetFreeStack();
    cur_blk_->AddInstruction(std::make_shared<StoreInstruction>(stk, name2ptr_[reg]));
    cur_blk_->instructions_.back()->comment_ = "CallerSaveRegisters: Save ft type register to stack";
    caller_saved_to_[reg] = stk;
  }
  for (auto &reg: used_fa_regs_) {
    auto stk = GetFreeStack();
    cur_blk_->AddInstruction(std::make_shared<StoreInstruction>(stk, name2ptr_[reg]));
    cur_blk_->instructions_.back()->comment_ = "CallerSaveRegisters: Save fa type register to stack";
    caller_saved_to_[reg] = stk;
  }

  // 将参数放入到指定的寄存器或者栈空间
  for (size_t i = 0; i < var2stk.size(); i++) {
    auto stk = var2stk[i];
    auto reg_name = static_cast<RegisterName>(RegisterName::A0 + i);
    cur_blk_->AddInstruction(std::make_shared<LoadInstruction>(name2ptr_[reg_name], stk));
    cur_blk_->instructions_.back()->comment_ = "SolveFuncCalling: Load the " + std::to_string(i) + "th arg to register";
    if (stk == nullptr) {
      throw std::runtime_error("SolveFuncCalling: stk == nullptr");
    }
    ReleaseStack(stk);
  }
  std::vector<Type> args_types = {};
  auto call = std::make_shared<CallInstruction>("memset", args_types);
  cur_blk_->AddInstruction(call);

  // 恢复现场
  for (auto &reg: used_t_regs_) {
    cur_blk_->AddInstruction(std::make_shared<LoadInstruction>(name2ptr_[reg], caller_saved_to_[reg]));
    cur_blk_->instructions_.back()->comment_ = "CallerLoadRegisters: Load t type register from stack";
    if (caller_saved_to_[reg] == nullptr) {
      throw std::runtime_error("CallerLoadRegisters: caller_saved_to_[reg] == nullptr");
    }
    ReleaseStack(caller_saved_to_[reg]);
  }
  for (auto &reg: used_a_regs_) {
    cur_blk_->AddInstruction(std::make_shared<LoadInstruction>(name2ptr_[reg], caller_saved_to_[reg]));
    cur_blk_->instructions_.back()->comment_ = "CallerLoadRegisters: Load a type register from stack";
    if (caller_saved_to_[reg] == nullptr) {
      throw std::runtime_error("CallerLoadRegisters: caller_saved_to_[reg] == nullptr");
    }
    ReleaseStack(caller_saved_to_[reg]);
  }
  for (auto &reg: used_ft_regs_) {
    cur_blk_->AddInstruction(std::make_shared<LoadInstruction>(name2ptr_[reg], caller_saved_to_[reg]));
    cur_blk_->instructions_.back()->comment_ = "CallerLoadRegisters: Load ft type register from stack";
    if (caller_saved_to_[reg] == nullptr) {
      throw std::runtime_error("CallerLoadRegisters: caller_saved_to_[reg] == nullptr");
    }
    ReleaseStack(caller_saved_to_[reg]);
  }
  for (auto &reg: used_fa_regs_) {
    cur_blk_->AddInstruction(std::make_shared<LoadInstruction>(name2ptr_[reg], caller_saved_to_[reg]));
    cur_blk_->instructions_.back()->comment_ = "CallerLoadRegisters: Load fa type register from stack";
    if (caller_saved_to_[reg] == nullptr) {
      throw std::runtime_error("CallerLoadRegisters: caller_saved_to_[reg] == nullptr");
    }
    ReleaseStack(caller_saved_to_[reg]);
  }
}

}