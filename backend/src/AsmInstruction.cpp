#include "../include/AsmInstruction.h"
#include "../include/riscv.h"

#include <sstream>

namespace SCNUCC {
auto JumpInstruction::ToString() -> std::string {
  std::stringstream os;
  os << "j " << target_blk_name_ << "     #" << comment_;
  return os.str();
}


auto BnezInstruction::ToString() -> std::string {
  std::stringstream os;
  os << "bnez " << reg2str[cond->reg_] << ", " << target_blk_name_ << "     #" << comment_;
  return os.str();
}


auto CallInstruction::ToString() -> std::string {
  std::stringstream os;
  os << "call " << func_name_ << "     #" << comment_;
  return os.str();
}


auto RetInstruction::ToString() -> std::string {
  std::stringstream os;
  os << "ret" << "     #" << comment_;
  return os.str();
}

auto LoadInstruction::ToString() -> std::string {
  std::stringstream os;
  if (target_->reg_ >= RegisterName::FT0 && target_->reg_ <= RegisterName::FA7 || load_float_) {
    os << "f"; 
  }
  if (stack_) {
    os << "ld " << reg2str[target_->reg_] << ", " << stack_->name_;
  } else {
    os << "ld " << reg2str[target_->reg_] << ", " << offset_ << "(" << reg2str[base_] << ")";
  }
  os << "     #" << comment_;
  return os.str();
}


auto StoreInstruction::ToString() -> std::string {
  std::stringstream os;
  if (src_->reg_ >= RegisterName::FT0 && src_->reg_ <= RegisterName::FA7) {
    os << "f"; 
  }
  if (target_) {
    os << "sd " << reg2str[src_->reg_] << ", " << target_->name_;
  } else {
    os << "sd " << reg2str[src_->reg_] << ", " << offset_ << "(" << reg2str[base_] << ")";
  }
  os << "     #" << comment_;
  return os.str();
}

auto StoreArgsInstruction::ToString() -> std::string {
  std::stringstream os;
  os << "sd " << reg2str[src_->reg_] << ", " << target_->offset_ << "(" << reg2str[RegisterName::SP] << ")";
  os << "     #" << comment_;
  return os.str();
}

auto BinaryInstruction::ToString() -> std::string {
  std::stringstream os;
  os << isn2str[type_] << " " << reg2str[target_->reg_] << ", " << reg2str[src1_->reg_] << ", " << reg2str[src2_->reg_];
  os << "     #" << comment_;
  return os.str();
}

auto BinaryImmInstruction::ToString() -> std::string {
  std::stringstream os;
  os << isn2str[type_] << " " << reg2str[target_->reg_] << ", " << reg2str[src_->reg_] << ", " << imm_->GetVal();
  os << "     #" << comment_;
  return os.str();
}

auto LoadImmInstruction::ToString() -> std::string {
  std::stringstream os;
  os << "li " << reg2str[target_->reg_] << ", " << imm_->GetVal();
  os << "     #" << comment_;
  return os.str();
}

auto LoadAddrInstruction::ToString() -> std::string {
  std::stringstream os;
  os << "la " << reg2str[target_->reg_] << ", " << label_.substr(1);
  os << "     #" << comment_;
  return os.str();
}

auto MoveInstruction::ToString() -> std::string {
  std::stringstream os;
  // mv rd, rs
  if (rd_->reg_ >= RegisterName::FT0 && rd_->reg_ <= RegisterName::FA7 && rs_->reg_ >= RegisterName::FT0 && rs_->reg_ <= RegisterName::FA7) {
    os << "fmv.d ";
  } else {
    os << "mv ";
  }
  os << reg2str[rd_->reg_] << ", " << reg2str[rs_->reg_];
  os << "     #" << comment_;
  return os.str();
}

auto LoadUpperInstruction::ToString() -> std::string {
  std::stringstream os;
  os << "lui " << reg2str[target_->reg_] << ", " << imm_->GetVal();
  os << "     #" << comment_;
  return os.str();
}

auto IntToFloatInstruction::ToString() -> std::string {
  std::stringstream os;
  os << "fcvt.d.l " << reg2str[target_->reg_] << ", " << reg2str[src_->reg_];
  os << "     #" << comment_;
  return os.str();
}

auto FloatToIntInstruction::ToString() -> std::string {
  std::stringstream os;
  os << "fcvt.l.d " << reg2str[target_->reg_] << ", " << reg2str[src_->reg_];
  os << "     #" << comment_;
  return os.str();
}

auto MoveIntToFloatInstruction::ToString() -> std::string {
  std::stringstream os;
  os << "fmv.d.x " << reg2str[target_->reg_] << ", " << reg2str[src_->reg_];
  os << "     #" << comment_;
  return os.str();
}

auto XorInstruction::ToString() -> std::string {
  std::stringstream os;
  os << "xor " << reg2str[target_->reg_] << ", " << reg2str[src1_->reg_] << ", " << reg2str[src2_->reg_];
  os <<"     #" << comment_;
  return os.str();
}

auto SNEZInstruction::ToString() -> std::string {
  std::stringstream os;
  os << "snez " << reg2str[rd_->reg_] << ", " << reg2str[rs_->reg_];
  os <<"     #" << comment_;
  return os.str();
}

auto XoriInstruction::ToString() -> std::string {
  std::stringstream os;
  os << "xori " << reg2str[rd_->reg_] << ", " << reg2str[rs_->reg_] << ", " << imm_->GetVal();
  os <<"     #" << comment_;
  return os.str();
}

// 需要区分int和float
auto CreateBinInstruction(const std::shared_ptr<IRStatement> &ir,
                          const std::vector<RegisterPtr> &regs) -> std::shared_ptr<BinaryInstruction> {
  auto op = ir->GetOp();
  AsmInstructionType type;
  // SEQZ , SNEZ, FEQZ, FNEZ ?
  switch (op) {
    case IROP::IR_ADD:
      type = AsmInstructionType::ADD;
      break;
    case IROP::IR_SUB:
      type = AsmInstructionType::SUB;
      break;
    case IROP::IR_ILT:
      type = AsmInstructionType::SLT;
      break;
    case IROP::IR_IGT:
      type = AsmInstructionType::SGT;
      break;
    case IROP::IR_XOR:
      type = AsmInstructionType::XOR;
      break;
    case IROP::IR_OR:
      type = AsmInstructionType::OR;
      break;
    case IROP::IR_AND:
      type = AsmInstructionType::AND;
      break;
    case IROP::IR_MUL:
      type = AsmInstructionType::MUL;
      break;
    case IROP::IR_SDIV:
      type = AsmInstructionType::DIV;
      break;
    case IROP::IR_SREM:
      type = AsmInstructionType::REM;
      break;
    case IROP::IR_IEQ:
      type = AsmInstructionType::SEQ;
      break;
    case IROP::IR_INE:
      type = AsmInstructionType::SNE;
      break;
    case IROP::IR_ILE:
      type = AsmInstructionType::SLE;
      break;
    case IROP::IR_IGE:
      type = AsmInstructionType::SGE;
      break;
    case IROP::IR_FADD:
      type = AsmInstructionType::FADD;
      break;
    case IROP::IR_FSUB:
      type = AsmInstructionType::FSUB;
      break;
    case IROP::IR_FDIV:
      type = AsmInstructionType::FDIV;
      break;
    case IROP::IR_FMUL:
      type = AsmInstructionType::FMUL;
      break;
    case IROP::IR_FLT:
      type = AsmInstructionType::FLT;
      break;
    case IROP::IR_FGT:
      type = AsmInstructionType::FGT;
      break;
    case IROP::IR_FEQ:
      type = AsmInstructionType::FEQ;
      break;
    case IROP::IR_FNE:
      type = AsmInstructionType::FNE;
      break;
    case IROP::IR_FLE:
      type = AsmInstructionType::FLE;
      break;
    case IROP::IR_FGE:
      type = AsmInstructionType::FGE;
      break;
    default:
      throw "CreateBinInstruction: Undefined AsmOP";
      break;
  }
  return std::make_shared<BinaryInstruction>(type, regs);
}

} // namespace SCNUCC

