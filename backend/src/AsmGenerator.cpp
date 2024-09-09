#include <sstream>
#include <queue>
#include <algorithm>
#include <cmath>
#include "../include/AsmGenerator.h"
#include "../include/Location.h"
#include "../include/MyVisitor.h"
#include "../include/AsmInstruction.h"
#include "../include/RegisterManager.h"
#include "../include/AsmBlock.h"
#include "../../frontend/include/IRVariable.h"
#include "../include/backend_util.h"

namespace SCNUCC {
std::string AsmGenerator::GenCode() {
  std::stringstream ss;
  if (!visitor_->gbl_vars.empty()) {
    ss << GlobalInit();
  }
  for (auto &cfg: visitor_->cfgs_) {
    Stack::stack_size_ = 16;  // 留给s0和ra寄存器寄存器
    auto reg_allocater = std::make_shared<RegisterAllocater>(cfg);
    auto var2loc = reg_allocater->AllocateRegister();
//     std::cout << "Register Allocation Result of " << cfg->func_name_ << std::endl;
//     std::cout << reg_allocater->ToString();
    block_map_.clear();
    cur_func_name_ = cfg->func_name_;
    auto init_block = std::make_shared<AsmBlock>(ConstructBlockName(cfg->blocks_[0]->GetSequenceID()), nullptr);
    init_block->block_flag_ = 1;
    block_map_[init_block->GetName()] = init_block;
    reg_manager_ = std::make_shared<RegisterManager>(var2loc, init_block);
    init_block->SetRegManager(reg_manager_.get());
    for (auto &block: cfg->blocks_) {
      auto asm_block = TranslateBlock(block);
    }

    for (auto &block: cfg->blocks_) {
      auto asm_block = block_map_[ConstructBlockName(block->GetSequenceID())];
      CriticalEdgeSplitting(asm_block);
    }

    reg_manager_->AlignSPWith16Byte();
    std::string allocate = "addi sp, sp, " + std::to_string(-(int) Stack::stack_size_) + "\n"
                          + "    sd s0, " + std::to_string(Stack::stack_size_ - 8) + "(sp)\n"
                          + "    addi s0, sp, " + std::to_string(Stack::stack_size_) + "\n"
                          + "    sd ra, " + std::to_string(Stack::stack_size_ - 16) + "(sp)";
    std::string destroy = "ld ra, " + std::to_string(Stack::stack_size_ - 16) + "(sp)\n"
                          + "    ld s0, " + std::to_string(Stack::stack_size_ - 8) + "(sp)\n"
                          + "    addi sp, sp, " + std::to_string(Stack::stack_size_);
    for (auto &item: block_map_) {
      auto blk = item.second;
      reg_manager_->SetCurBlock(blk);
      ss << blk->ToString(allocate, destroy);
    }
  }
  return ss.str();
}

auto AsmGenerator::ConstructBlockName(size_t block_id) -> std::string {
  if (block_id == 1) {
    return cur_func_name_;
  }
  return cur_func_name_ + "_block_" + std::to_string(block_id - 1);
}

auto AsmGenerator::GlobalInit() -> std::string {
  std::stringstream ss;
  for (auto &[name, ir]: visitor_->gbl_vars) {
    auto var = ir->GetResultVar();
    var->home_location_ = std::shared_ptr<Location>(new Heap(0, var->name_));
    if (var->type_ == IRVariable::Type::INT_IMM || var->type_ == IRVariable::Type::INT) {
      int val = ir->GetResultVar()->int_value_;
      if (val == 0) {
        ss << ".bss\n" << name << ":\n";
        ss << "\t.skip  8\n";
      } else {
        ss << ".data\n" << name << ":\n";
        ss << "\t.dword  " << val << "\n";
      }
    } else if (var->type_ == IRVariable::Type::FLOAT_IMM || var->type_ == IRVariable::Type::FLOAT) {
      float val = ir->GetResultVar()->float_value_;
      if (val == 0.0) {
        ss << ".bss\n" << name << ":\n";
        ss << "\t.skip  8\n";
      } else {
        ss << ".data\n" << name << ":\n";
        ss << "\t.dword  " << Util::GetIEEE75416(val) << "\n";
      }
    } else if (var->type_ == IRVariable::Type::INT_POINTER || var->type_ == IRVariable::Type::FLOAT_POINTER) {
      auto arr_ir = std::dynamic_pointer_cast<GblVarDeclIR>(ir);
      auto sym = arr_ir->GetSym();
      auto array_shape = sym->GetType().array_shape_;
      if (arr_ir->AllZero()) {
        size_t size = 8;
        for (auto it: array_shape) {
          size *= it;
        }
        ss << ".data\n" << name << ":\n";
        ss << "\t.zero  " << size << "\n";
      } else {
        ss << ".data\n" << name << ":\n";
        auto vt = std::any_cast<std::vector<std::any>>(sym->GetArrayVal());
        if (var->type_ == IRVariable::Type::INT_POINTER) {
          ss << GlobalArrayInit(vt, array_shape, 0, 1);
        } else {
          ss << GlobalArrayInit(vt, array_shape, 0, 0);
        }
      }
    } else {
      throw "AsmGenerator::GlobalInit: Unknown type";
    }
  }
  return ss.str();
}

auto AsmGenerator::GlobalArrayInit(std::vector<std::any> &values, std::vector<size_t> &indices, int cur,
                                   int is_int) -> std::string {
  int size = 8;
  int idx = cur;
  while (idx < indices.size()) {
    size *= indices[idx++];
  }
  std::stringstream ss;
  if (values.empty()) {
    ss << "\t.zero\t" << size;
    return ss.str();
  }
  if (values.at(0).type() != typeid(std::vector<std::any>)) {
    for (auto &val: values) {
      ss << "\t.dword  ";
      auto ir = std::any_cast<std::shared_ptr<IRStatement>>(val);
      if (is_int) {
        ss << std::any_cast<int>(ir->GetConstantVal()) << "\n";
      } else {
        float num = std::any_cast<float>(ir->GetConstantVal());
        ss << SCNUCC::GetIEEE754(num) << "\n";
      }
      size -= 8;
    }
    if (size > 0) {
      ss << "\t.zero  " << size << "\n";
    }
  } else {
    int base = 8, sum = 0;
    idx = cur + 1;
    while (idx < indices.size()) {
      base *= indices[idx++];
    }
    for (auto &val: values) {
      sum += base;
      auto vt = std::any_cast<std::vector<std::any>>(val);
      ss << GlobalArrayInit(vt, indices, cur + 1, is_int);
    }
    if (sum < size) {
      ss << "\n\t.zero  " << size - sum << "\n";
    }
  }
  return ss.str();
}


auto AsmGenerator::TranslateBlock(std::shared_ptr<Block> &block) -> std::shared_ptr<AsmBlock> {
  auto cur_blk_name = ConstructBlockName(block->GetSequenceID());
  std::shared_ptr<AsmBlock> asm_blk;
  if (block_map_.find(cur_blk_name) == block_map_.end()) {
    asm_blk = std::make_shared<AsmBlock>(cur_blk_name, reg_manager_.get());
    block_map_[cur_blk_name] = asm_blk;
  } else {
    asm_blk = block_map_[cur_blk_name];
  }
  reg_manager_->SetCurBlock(asm_blk);

  for (auto &stmt: block->GetStmts()) {
    auto op = stmt->GetOp();
    // 整数加减乘除
    if (op == IROP::IR_INE) {
      // 就是说两个元素xor，如果不为0，那么其实就是不相等的
      std::vector<IRVariablePtr> vars = {stmt->GetResultVar(), stmt->GetOperands()[0], stmt->GetOperands()[1]};
      auto locs = reg_manager_->AssignRegister(vars);
      auto tmp_reg = reg_manager_->GetFreeIntReg();
      tmp_reg->new_data_home_ = nullptr;
      // xor t0 t1 t2
      auto xor_instr = std::make_shared<XorInstruction>(tmp_reg, locs[1], locs[2]);
      asm_blk->AddInstruction(xor_instr);
      // snez t3 t0  到这里，如果两个元素相等，那么t3 = 0, 否则t3 = 1
      auto snez = std::make_shared<SNEZInstruction>(locs[0], tmp_reg);
      asm_blk->AddInstruction(snez);
      reg_manager_->ReleaseRegister(tmp_reg);
      reg_manager_->ReleaseRegister(locs);
    } else if (op == IROP::IR_IEQ) {
      // 就是说两个元素xor，如果不为0，那么其实就是不相等的
      std::vector<IRVariablePtr> vars = {stmt->GetResultVar(), stmt->GetOperands()[0], stmt->GetOperands()[1]};
      auto locs = reg_manager_->AssignRegister(vars);
      auto tmp_reg = reg_manager_->GetFreeIntReg();
      tmp_reg->new_data_home_ = nullptr;
      // xor t0 t1 t2
      auto xor_instr = std::make_shared<XorInstruction>(tmp_reg, locs[1], locs[2]);
      asm_blk->AddInstruction(xor_instr);
      // snez t0 t0  到这里，如果两个元素相等，那么t0 = 0, 否则t0 = 1
      auto snez = std::make_shared<SNEZInstruction>(tmp_reg, tmp_reg);
      asm_blk->AddInstruction(snez);
      auto one = std::make_shared<Immediate12>(1, 1);
      // 再拿t0和1进行xor，如果两个元素相等，那么xor = 1, 否者xor = 0
      auto xori_instr = std::make_shared<XoriInstruction>(locs[0], tmp_reg, one);
      asm_blk->AddInstruction(xori_instr);
      reg_manager_->ReleaseRegister(tmp_reg);
      reg_manager_->ReleaseRegister(locs);
    } else if (op == IROP::IR_IGE) {
      std::vector<IRVariablePtr> vars = {stmt->GetResultVar(), stmt->GetOperands()[0], stmt->GetOperands()[1]};
      auto locs = reg_manager_->AssignRegister(vars);
      auto tmp_reg = reg_manager_->GetFreeIntReg();
      tmp_reg->new_data_home_ = nullptr;
      // 小于
      auto slt = std::make_shared<BinaryInstruction>(AsmInstructionType::SLT, tmp_reg, locs[1], locs[2]);
      asm_blk->AddInstruction(slt);
      // 再和1进行xor，如果小于，那么xor结果是0，如果大于等于，那么结果是1
      auto one = std::make_shared<Immediate12>(1, 1);
      auto xori_instr = std::make_shared<XoriInstruction>(locs[0], tmp_reg, one);
      asm_blk->AddInstruction(xori_instr);
      reg_manager_->ReleaseRegister(tmp_reg);
      reg_manager_->ReleaseRegister(locs);
    } else if (op == IROP::IR_ILE) {
      std::vector<IRVariablePtr> vars = {stmt->GetResultVar(), stmt->GetOperands()[0], stmt->GetOperands()[1]};
      auto locs = reg_manager_->AssignRegister(vars);
      auto tmp_reg = reg_manager_->GetFreeIntReg();
      tmp_reg->new_data_home_ = nullptr;
      // 大于
      auto sgt = std::make_shared<BinaryInstruction>(AsmInstructionType::SGT, tmp_reg, locs[1], locs[2]);
      asm_blk->AddInstruction(sgt);
      // 如果a > b，那么结果是1，否则是0
      // 此时再和1进行xor，如果大于，那么xor为0，如果小于等于，那么xor为1
      auto one = std::make_shared<Immediate12>(1, 1);
      auto xori_instr = std::make_shared<XoriInstruction>(locs[0], tmp_reg, one);
      asm_blk->AddInstruction(xori_instr);
      reg_manager_->ReleaseRegister(tmp_reg);
      reg_manager_->ReleaseRegister(locs);
    } else if (op == IROP::IR_FGE) {
      std::vector<IRVariablePtr> vars = {stmt->GetResultVar(), stmt->GetOperands()[0], stmt->GetOperands()[1]};
      auto locs = reg_manager_->AssignRegister(vars);
      auto tmp_reg = reg_manager_->GetFreeIntReg();
      tmp_reg->new_data_home_ = nullptr;
      auto flt = std::make_shared<BinaryInstruction>(AsmInstructionType::FLT, tmp_reg, locs[1], locs[2]);
      asm_blk->AddInstruction(flt);
      auto one = std::make_shared<Immediate12>(1, 1);
      auto xori_instr = std::make_shared<XoriInstruction>(locs[0], tmp_reg, one);
      asm_blk->AddInstruction(xori_instr);
      reg_manager_->ReleaseRegister(tmp_reg);
      reg_manager_->ReleaseRegister(locs);
    } else if (op == IROP::IR_FLE) {
      std::vector<IRVariablePtr> vars = {stmt->GetResultVar(), stmt->GetOperands()[0], stmt->GetOperands()[1]};
      auto locs = reg_manager_->AssignRegister(vars);
      auto tmp_reg = reg_manager_->GetFreeIntReg();
      tmp_reg->new_data_home_ = nullptr;
      // greater
      auto fgt = std::make_shared<BinaryInstruction>(AsmInstructionType::FGT, tmp_reg, locs[1], locs[2]);
      asm_blk->AddInstruction(fgt);
      auto one = std::make_shared<Immediate12>(1, 1);
      // 跟1进行xor
      auto xori_instr = std::make_shared<XoriInstruction>(locs[0], tmp_reg, one);
      asm_blk->AddInstruction(xori_instr);
      reg_manager_->ReleaseRegister(tmp_reg);
      reg_manager_->ReleaseRegister(locs);
    } else if (op == IROP::IR_FNE) {
      std::vector<IRVariablePtr> vars = {stmt->GetResultVar(), stmt->GetOperands()[0], stmt->GetOperands()[1]};
      auto locs = reg_manager_->AssignRegister(vars);
      auto tmp_reg = reg_manager_->GetFreeIntReg();
      tmp_reg->new_data_home_ = nullptr;
      auto feq = std::make_shared<BinaryInstruction>(AsmInstructionType::FEQ, tmp_reg, locs[1], locs[2]);
      auto one = std::make_shared<Immediate12>(1, 1);
      // 跟1进行xor
      auto xori_instr = std::make_shared<XoriInstruction>(locs[0], tmp_reg, one);
      asm_blk->AddInstruction(xori_instr);
      reg_manager_->ReleaseRegister(tmp_reg);
      reg_manager_->ReleaseRegister(locs);
    } else if (op == IROP::IR_ADD || op == IROP::IR_SUB || op == IROP::IR_MUL || op == IROP::IR_SDIV ||
               op == IROP::IR_SREM
               // 整数的比较运算符
               || op == IROP::IR_IGT || op == IROP::IR_ILT
               // 逻辑运算符
               || op == IROP::IR_AND || op == IROP::IR_NOT || op == IROP::IR_OR
               // 浮点数运算符
               || op == IROP::IR_FADD || op == IROP::IR_FSUB || op == IROP::IR_FMUL || op == IROP::IR_FDIV ||
               op == IROP::IR_FREM
               // 浮点数比较运算符
               || op == IROP::IR_FGT || op == IROP::IR_FGE || op == IROP::IR_FLT || op == IROP::IR_FLE ||
               op == IROP::IR_FEQ) {
      std::vector<IRVariablePtr> vars = {stmt->GetResultVar(), stmt->GetOperands()[0], stmt->GetOperands()[1]};
      auto locs = reg_manager_->AssignRegister(vars);
      auto asm_instr = CreateBinInstruction(stmt, locs);
      asm_blk->AddInstruction(asm_instr);
      reg_manager_->ReleaseRegister(locs);
    } else if (op == IROP::IR_XOR) {
      std::vector<IRVariablePtr> vars = {stmt->GetResultVar(), stmt->GetOperands()[0]};
      auto locs = reg_manager_->AssignRegister(vars);
      auto imm = std::make_shared<Immediate12>(1, true);
      auto instr = std::make_shared<BinaryImmInstruction>(AsmInstructionType::XORI, locs[0], locs[1], imm);
      asm_blk->AddInstruction(instr);
      reg_manager_->ReleaseRegister(locs);
    } else if (op == IROP::IR_ITOF) {
      std::vector<IRVariablePtr> vars = {stmt->GetResultVar(), stmt->GetOperands()[0]};
      auto locs = reg_manager_->AssignRegister(vars);
      auto asm_instr = std::make_shared<IntToFloatInstruction>(locs[1], locs[0]);
      asm_blk->AddInstruction(asm_instr);
      reg_manager_->ReleaseRegister(locs);
    } else if (op == IROP::IR_FTOI) {
      std::vector<IRVariablePtr> vars = {stmt->GetResultVar(), stmt->GetOperands()[0]};
      auto locs = reg_manager_->AssignRegister(vars);
      auto asm_instr = std::make_shared<FloatToIntInstruction>(locs[1], locs[0]);
      asm_blk->AddInstruction(asm_instr);
      reg_manager_->ReleaseRegister(locs);
    } else if (op == IROP::IR_BTOI) {
      // TODO: 好像不需要做什么事情
      // 使用andi dst src 1，保留最后一位，进行扩展，保险一点
      std::vector<IRVariablePtr> vars = {stmt->GetResultVar(), stmt->GetOperands()[0]};
      auto locs = reg_manager_->AssignRegister(vars);
      auto one = std::make_shared<Immediate12>(1, 1);
      auto andi = std::make_shared<BinaryImmInstruction>(AsmInstructionType::ANDI, locs[0], locs[1], one);
      asm_blk->AddInstruction(andi);
      reg_manager_->ReleaseRegister(locs);
    } else if (op == IROP::IR_NEG) {
      // 这里其实我们并没有用上NEG的IR，所以这个翻译是没有用的
      // 我们在翻译-a的时候，翻译成了算术运算 0 - a，是减法操作
      std::vector<IRVariablePtr> vars = {stmt->GetResultVar(), stmt->GetOperands()[0]};
      auto locs = reg_manager_->AssignRegister(vars);
      auto zero = reg_manager_->GetSpecRegister(RegisterName::ZERO);
      auto asm_instr = std::make_shared<BinaryInstruction>(AsmInstructionType::SUB, locs[0], zero, locs[1]);
      asm_blk->AddInstruction(asm_instr);
      reg_manager_->ReleaseRegister(locs);
    } else if (op == IROP::IR_ALLOCA) { // 访存操作
      auto ir = std::dynamic_pointer_cast<ArrayDeclIR>(stmt);
      if (ir == nullptr) {
        throw std::runtime_error("Not Implemented Alloca");
        auto ir = std::dynamic_pointer_cast<AllocaIR>(stmt);
        auto var = ir->GetResultVar();
        std::vector<IRVariablePtr> vars = {var};
        auto reg = reg_manager_->AssignRegister(vars)[0];
      } else { 
        // 保存a0, a1, a2三个寄存器，调用一个memset
        reg_manager_->SolveMemset(ir);
      }

    } else if (op == IROP::IR_STR) {
      auto ir = std::dynamic_pointer_cast<StoreIR>(stmt);
      auto locs = reg_manager_->AssignRegister(ir->GetOperands());  // data, pointer
      auto store = std::make_shared<StoreInstruction>(locs[1]->reg_, 0, locs[0]);
      asm_blk->AddInstruction(store);
      reg_manager_->ReleaseRegister(locs);
    } else if (op == IROP::IR_LRD) {
      auto ir = std::dynamic_pointer_cast<LoadIR>(stmt);
      std::vector<IRVariablePtr> vars = {ir->GetResultVar(), ir->GetOperands()[0]};
      auto locs = reg_manager_->AssignRegister(vars);  // data, pointer
      asm_blk->AddInstruction(std::make_shared<LoadInstruction>(locs[0], locs[1]->reg_, 0));
      reg_manager_->ReleaseRegister(locs);
    } else if (op == IROP::IR_BR) {
      auto ir = std::dynamic_pointer_cast<BRIR>(stmt);
      if (stmt->GetOperands().size() == 1) {  // 无条件跳转
        auto true_blk_decl = std::dynamic_pointer_cast<BlockDeclIR>(ir->GetTrueBranch());
        auto true_blk_name = ConstructBlockName(true_blk_decl->blk_->GetSequenceID());
        if (block_map_.find(true_blk_name) == block_map_.end()) {  // 构建跳转关系
          auto child = std::make_shared<AsmBlock>(true_blk_name, reg_manager_.get());
          block_map_[true_blk_name] = child;
        }
        block_map_[true_blk_name]->AddParent(asm_blk);
        asm_blk->SetTrueBranch(block_map_[true_blk_name]);
      } else { // 有条件跳转
        auto true_blk_decl = std::dynamic_pointer_cast<BlockDeclIR>(ir->GetTrueBranch());
        auto true_blk_name = ConstructBlockName(true_blk_decl->blk_->GetSequenceID());
        auto false_blk_decl = std::dynamic_pointer_cast<BlockDeclIR>(ir->GetFalseBranch());
        auto false_blk_name = ConstructBlockName(false_blk_decl->blk_->GetSequenceID());
        std::vector<IRVariablePtr> vars = {ir->GetCond()->GetResultVar()};
        auto locs = reg_manager_->AssignRegister(vars);
        asm_blk->SetJumpCond(locs[0]->reg_);

        std::shared_ptr<AsmBlock> child;
        if (block_map_.find(true_blk_name) == block_map_.end()) {  // 构建跳转关系
          child = std::make_shared<AsmBlock>(true_blk_name, reg_manager_.get());
          block_map_[true_blk_name] = child;
        } else {
          child = block_map_[true_blk_name];
        }
        asm_blk->SetTrueBranch(child);
        child->AddParent(asm_blk);

        if (block_map_.find(false_blk_name) == block_map_.end()) {  // 构建跳转关系
          child = std::make_shared<AsmBlock>(false_blk_name, reg_manager_.get());
          block_map_[false_blk_name] = child;
        } else {
          child = block_map_[false_blk_name];
        }
        asm_blk->SetFalseBranch(child);
        child->AddParent(asm_blk);
        reg_manager_->ReleaseRegister(locs);
      }
    } else if (op == IROP::IR_RET) {
      if (stmt->GetOperands().size() == 1) {
        auto locs = reg_manager_->AssignRegister(stmt->GetOperands());
        auto var = stmt->GetOperands()[0];
        if (!var->IsImm()) {
          locs[0]->new_data_home_ = std::static_pointer_cast<Stack>(stmt->GetOperands()[0]->home_location_);
        }
        if (locs[0]->reg_ >= RegisterName::T0 && locs[0]->reg_ <= RegisterName::A7) {
          auto a0 = reg_manager_->GetSpecRegister(RegisterName::A0);
          asm_blk->AddInstruction(std::make_shared<MoveInstruction>(a0, locs[0]));
        } else {
          auto fa0 = reg_manager_->GetSpecRegister(RegisterName::FA0);
          asm_blk->AddInstruction(std::make_shared<MoveInstruction>(fa0, locs[0]));
        }
        asm_blk->instructions_.back()->comment_ = "TranslateBlock: Move the result to A0";
        reg_manager_->ReleaseRegister(locs);
      }
      asm_blk->block_flag_ = 2;
      reg_manager_->CalleeLoadRegisters();
      asm_blk->AddInstruction(std::make_shared<RetInstruction>());
    } else if (op == IROP::IR_CALL) {
      auto ir = std::dynamic_pointer_cast<CallIR>(stmt);
      reg_manager_->SolveFuncCalling(ir);
    } else if (op == IROP::IR_PHI) {
      asm_blk->AddPhi(std::dynamic_pointer_cast<PhiIR>(stmt));
    } else if (op == IROP::IR_PLACEHOLDER) {
      throw std::runtime_error("Placeholder Should not Appear");
    } else if (op == IROP::IR_BITCAST) {
//      throw std::runtime_error("Bitcast Not implemented yet");
      // do nothing
    } else if (op == IROP::IR_GEP) {
      auto ir = std::dynamic_pointer_cast<GEPIR>(stmt);
      auto oprand_0 = stmt->GetOperands()[0];
      auto oprand_1 = stmt->GetOperands()[1];
      std::vector<IRVariablePtr> vars = {stmt->GetResultVar(), oprand_0, oprand_1};
      // GEPIR
      auto locs = reg_manager_->AssignRegister(vars);
      // get 第一维度
      if (std::dynamic_pointer_cast<ArrayDeclIR>(oprand_0->defined_in_) 
          || (std::dynamic_pointer_cast<PhiIR>(oprand_0->defined_in_) && oprand_0->array_location_ != nullptr)) {
        auto stk = std::dynamic_pointer_cast<Stack>(oprand_0->array_location_);
        int offset_num = std::abs(stk->offset_);
        auto s0 = reg_manager_->GetSpecRegister(RegisterName::S0);
        if (offset_num < 2040) {
          auto offset = std::make_shared<Immediate12>(stk->offset_, 1);
          auto addi = std::make_shared<BinaryImmInstruction>(AsmInstructionType::ADDI, locs[1], s0, offset);
          asm_blk->AddInstruction(addi);
        } else {
          auto offset_32 = std::make_shared<Immediate32>(offset_num, 1);
          auto li_offset = std::make_shared<LoadImmInstruction>(locs[1], offset_32);
          asm_blk->AddInstruction(li_offset);
          auto add_offset = std::make_shared<BinaryInstruction>(AsmInstructionType::ADD, locs[1], locs[1], s0);
          asm_blk->AddInstruction(add_offset);
        }
      } // 还要判断global_decl_ir的
      // a[1]
      // %7 = getelementptr inbounds [2 x i32], ptr %6, i64 0, i64 1
      // locs[1]是ptr %6, locs[2]是最后面那个1，第二个元素
      // locs[2]乘4，表示字节，申请一个SLLI 左移三位
      auto constant = std::make_shared<Immediate12>(3, 1);  // 左移三位，乘八
      int left_num = ir->LeftSize();  // 没有乘八, 后面的维度数相乘
      auto left_imm = std::make_shared<Immediate32>(left_num, 1);
      // 拿一个寄存器
      auto index_reg = reg_manager_->GetFreeIntReg();
      index_reg->new_data_home_ = nullptr;

      auto load_left_num = std::make_shared<LoadImmInstruction>(index_reg, left_imm);
      asm_blk->AddInstruction(load_left_num);
      // locs[2]是当前数组的下标，不能被改边，因为他是下标值
      // index_reg是下标的字节表示，抢占来的寄存器可以改变，因为会复原
      auto mul = std::make_shared<BinaryInstruction>(AsmInstructionType::MUL, index_reg, locs[2], index_reg);
      auto slli = std::make_shared<BinaryImmInstruction>(AsmInstructionType::SLLI, index_reg, index_reg, constant);
      auto add = std::make_shared<BinaryInstruction>(AsmInstructionType::ADD, locs[0], locs[1], index_reg);
      asm_blk->AddInstruction(mul);
      asm_blk->AddInstruction(slli);
      asm_blk->AddInstruction(add);
      reg_manager_->ReleaseRegister(index_reg);
      reg_manager_->ReleaseRegister(locs);


    } else if (op == IROP::IR_BLOCKDECL) {
      // do nothing
    } else {
      std::string msg = "Unknown IR Type: " + std::to_string(op);
      throw std::runtime_error(msg);
    }
  }

  return asm_blk;
}

auto AsmGenerator::CriticalEdgeSplitting(std::shared_ptr<AsmBlock> &cur_blk) -> void {
  if (cur_blk->phi_irs_.empty()) {
    return;
  }
  auto blkname2pc = std::map<std::string, std::shared_ptr<ParallelCopy>>();
  auto parents = cur_blk->parents_;
  for (auto &parent: parents) {
    if (parent->GetChildNum() > 1) {
      auto new_blk = std::make_shared<AsmBlock>(parent->GetName() + cur_blk->GetName() + "copy_blk",
                                                reg_manager_.get());
      // 建立parent和new_blk的链接
      parent->ChangeBranch(cur_blk, new_blk);
      new_blk->AddParent(parent);

      // 建立new_blk和cur_blk的链接
      cur_blk->parents_.erase(parent);
      cur_blk->AddParent(new_blk);
      new_blk->SetTrueBranch(cur_blk);
      block_map_[new_blk->GetName()] = new_blk;

      blkname2pc[parent->GetName()] = new_blk->GetParallelCopy();
    } else {
      blkname2pc[parent->GetName()] = parent->parallel_copy_;
    }
  }

  for (auto &phi: cur_blk->phi_irs_) {
    auto operands = phi->GetOperands();
    auto dest_var = phi->GetResultVar();
    auto dest_loc = dest_var->home_location_;
    for (size_t i = 0; i < operands.size(); i += 2) {
      auto src_var = operands[i];
      auto src_blk_name = ConstructBlockName(
              std::dynamic_pointer_cast<BlockDeclIR>(operands[i + 1]->defined_in_)->blk_->GetSequenceID());
      blkname2pc[src_blk_name]->AddCopy(src_var->home_location_, dest_loc);
    }
  }
}

} // namespace SCNUCC