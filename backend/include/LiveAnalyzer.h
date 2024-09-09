//
// 
// 定义LiveAnalyzer主要进行活跃变量分析

#ifndef SCNUCC_LIVEANALYZER_H
#define SCNUCC_LIVEANALYZER_H

#include <memory>
#include <map>
#include "../../frontend/include/ControlFlowGraph.h"

class Block;

class IRVariable;

namespace SCNUCC {
class LiveAnalyzer {
public:
  typedef std::shared_ptr<IRVariable> IRVariablePtr;
  typedef std::set<IRVariablePtr> IRVariableSet;
  typedef std::pair<size_t, size_t> Range;

  std::shared_ptr<ControlFlowGraph> cfg_;

  std::map<std::shared_ptr<Block>, bool> is_processed_;

  std::map<std::shared_ptr<Block>, IRVariableSet> uses_;
  std::map<std::shared_ptr<Block>, IRVariableSet> defs_;
  std::map<std::shared_ptr<Block>, IRVariableSet> live_in_;
  std::map<std::shared_ptr<Block>, IRVariableSet> live_out_;
  std::map<std::shared_ptr<Block>, IRVariableSet> f_uses_;
  std::map<std::shared_ptr<Block>, IRVariableSet> f_defs_;
  std::map<std::shared_ptr<Block>, IRVariableSet> f_live_in_;
  std::map<std::shared_ptr<Block>, IRVariableSet> f_live_out_;

  std::map<std::shared_ptr<IRVariable>, std::set<Range>> var2ranges_;
  std::map<std::shared_ptr<IRVariable>, std::set<Range>> f_var2ranges_;

  LiveAnalyzer() = default;

  ~LiveAnalyzer() = default;

  auto Analyze() -> std::pair<std::map<IRVariablePtr, Range>, std::map<IRVariablePtr, Range>>;

  auto CalDefsUses() -> void;

  auto CalLiveInOut() -> void;
};

} // namespace SCNUCC

#endif //SCNUCC_LIVEANALYZER_H
