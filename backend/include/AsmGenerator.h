/**
 * 定义AsmGenerator类，是RISCV代码生成器
 */

#ifndef SCNUCC_ASMGENERATOR_H
#define SCNUCC_ASMGENERATOR_H

#include "../../frontend/include/MyVisitor.h"
#include "../../frontend/include/ControlFlowGraph.h"
#include "./RegisterAllocater.h"
#include "./riscv.h"
#include "./Location.h"
#include <memory>
#include <any>


namespace SCNUCC {
using IRPtr = std::shared_ptr<IRStatement>;

class AsmInstruction;

class AsmBlock;

class RegisterManager;

/**
 * 生成RISCV代码的生成器
 */
class AsmGenerator {
private:
  std::shared_ptr<SCNUCC::MyVisitor> visitor_;
  std::shared_ptr<RegisterManager> reg_manager_;
  std::shared_ptr<AsmBlock> root_block_;
  std::map<std::string, std::shared_ptr<AsmBlock>> block_map_;
  std::string cur_func_name_;

  auto ConstructBlockName(size_t block_id) -> std::string;

  auto TranslateBlock(std::shared_ptr<Block> &block) -> std::shared_ptr<AsmBlock>;

  auto CriticalEdgeSplitting(std::shared_ptr<AsmBlock> &cur_blk) -> void;

  auto GlobalInit() -> std::string;

  auto GlobalArrayInit(std::vector<std::any> &values, std::vector<size_t> &indices, int cur, int is_int) -> std::string;

public:

  explicit AsmGenerator(const std::shared_ptr<SCNUCC::MyVisitor> &visitor) : visitor_(visitor) {}

  ~AsmGenerator() = default;

  auto GenCode() -> std::string;
};

}

#endif //SCNUCC_ASMGENERATOR_H
