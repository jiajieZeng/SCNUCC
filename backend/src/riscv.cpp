#include "../include/riscv.h"

namespace SCNUCC {
std::map<RegisterName, std::string> reg2str = {
        {RegisterName::ZERO, "zero"},
        {RegisterName::RA,   "ra"},
        {RegisterName::SP,   "sp"},
        {RegisterName::GP,   "gp"},
        {RegisterName::TP,   "tp"},
        {RegisterName::T0,   "t0"},
        {RegisterName::T1,   "t1"},
        {RegisterName::T2,   "t2"},
        {RegisterName::T3,   "t3"},
        {RegisterName::T4,   "t4"},
        {RegisterName::T5,   "t5"},
        {RegisterName::T6,   "t6"},
        {RegisterName::S0,   "s0"},
        {RegisterName::S1,   "s1"},
        {RegisterName::S2,   "s2"},
        {RegisterName::S3,   "s3"},
        {RegisterName::S4,   "s4"},
        {RegisterName::S5,   "s5"},
        {RegisterName::S6,   "s6"},
        {RegisterName::S7,   "s7"},
        {RegisterName::S8,   "s8"},
        {RegisterName::S9,   "s9"},
        {RegisterName::S10,  "s10"},
        {RegisterName::S11,  "s11"},
        {RegisterName::A0,   "a0"},
        {RegisterName::A1,   "a1"},
        {RegisterName::A2,   "a2"},
        {RegisterName::A3,   "a3"},
        {RegisterName::A4,   "a4"},
        {RegisterName::A5,   "a5"},
        {RegisterName::A6,   "a6"},
        {RegisterName::A7,   "a7"},

        {RegisterName::FT0,  "ft0"},
        {RegisterName::FT1,  "ft1"},
        {RegisterName::FT2,  "ft2"},
        {RegisterName::FT3,  "ft3"},
        {RegisterName::FT4,  "ft4"},
        {RegisterName::FT5,  "ft5"},
        {RegisterName::FT6,  "ft6"},
        {RegisterName::FT7,  "ft7"},
        {RegisterName::FT8,  "ft8"},
        {RegisterName::FT9,  "ft9"},
        {RegisterName::FT10, "ft10"},
        {RegisterName::FT11, "ft11"},

        {RegisterName::FS0,  "fs0"},
        {RegisterName::FS1,  "fs1"},
        {RegisterName::FS2,  "fs2"},
        {RegisterName::FS3,  "fs3"},
        {RegisterName::FS4,  "fs4"},
        {RegisterName::FS5,  "fs5"},
        {RegisterName::FS6,  "fs6"},
        {RegisterName::FS7,  "fs7"},
        {RegisterName::FS8,  "fs8"},
        {RegisterName::FS9,  "fs9"},
        {RegisterName::FS10, "fs10"},
        {RegisterName::FS11, "fs11"},

        {RegisterName::FA0,  "fa0"},
        {RegisterName::FA1,  "fa1"},
        {RegisterName::FA2,  "fa2"},
        {RegisterName::FA3,  "fa3"},
        {RegisterName::FA4,  "fa4"},
        {RegisterName::FA5,  "fa5"},
        {RegisterName::FA6,  "fa6"},
        {RegisterName::FA7,  "fa7"},
};

std::map<AsmInstructionType, std::string> isn2str = {
  {AsmInstructionType::JUMP, "j"},
  {AsmInstructionType::BEQZ, "beqz"},
  {AsmInstructionType::BNEZ, "bnez"},
  {AsmInstructionType::CALL, "call"},
  {AsmInstructionType::RET, "ret"},

  {AsmInstructionType::LW, "lw"},
  {AsmInstructionType::SW, "sw"},

  {AsmInstructionType::ADD, "add"},// 加法
  {AsmInstructionType::ADDI, "addi"}, // 加法立即数
  {AsmInstructionType::SUB, "sub"},// 减法
  {AsmInstructionType::SLT, "slt"},// 小于
  {AsmInstructionType::SGT, "sgt"},// 大于
  {AsmInstructionType::SEQZ, "seqz"},
  {AsmInstructionType::SNEZ, "snez"},
  {AsmInstructionType::XOR, "xor"},
  {AsmInstructionType::XORI, "xori"},
  {AsmInstructionType::OR, "or"},
  {AsmInstructionType::ORI, "ori"}, // ?
  {AsmInstructionType::AND, "and"},
  {AsmInstructionType::ANDI, "andi"},
  {AsmInstructionType::SLL, "sll"},
  {AsmInstructionType::SRL, "srl"},
  {AsmInstructionType::SRA, "sra"},
  {AsmInstructionType::MUL, "mul"},
  {AsmInstructionType::DIV, "div"},
  {AsmInstructionType::REM, "rem"},
  
  {AsmInstructionType::SEQ, "beq"},
  {AsmInstructionType::SNE, "bne"},
  {AsmInstructionType::SLE, "ble"}, // ?
  {AsmInstructionType::SGE, "bge"},
  {AsmInstructionType::FADD, "fadd.d"},
  {AsmInstructionType::FSUB, "fsub.d"},
  {AsmInstructionType::FDIV, "fdiv.d"},
  {AsmInstructionType::FMUL, "fmul.d"},
  {AsmInstructionType::FLT, "flt.d"},
  {AsmInstructionType::FGT, "fgt.d"},
  {AsmInstructionType::FEQZ, "feqz.d"},
  {AsmInstructionType::FNEZ, "fnez.d"},
  {AsmInstructionType::FEQ, "feq.d"},
  {AsmInstructionType::FNE, "fne.d"}, // ?
  {AsmInstructionType::FLE, "fle.d"},
  {AsmInstructionType::FGE, "fge.d"},

  {AsmInstructionType::LI, "li"},
  {AsmInstructionType::MV, "mv"},
  {AsmInstructionType::LA, "la"},
  {AsmInstructionType::LUI, "lui"},
  {AsmInstructionType::SLLI, "slli"},
  {AsmInstructionType::SLTU, "sltu"},
};

} // namespace SCNUCC