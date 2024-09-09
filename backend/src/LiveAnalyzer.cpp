#include <algorithm>
#include "../../frontend/include/IRStatement.h"
#include "../../frontend/include/IRVariable.h"
#include "../include/LiveAnalyzer.h"

namespace SCNUCC {

// auto LiveAnalyzer::CalDefsUses() -> void {
//   for (auto &block: cfg_->blocks_) {
//     if (block->GetSequenceID() == 2) {
//       int xx = 1;
//     }
//     auto &uses = uses_[block];
//     auto &defs = defs_[block];
//     auto &f_uses = f_uses_[block];
//     auto &f_defs = f_defs_[block];
//     auto &live_in = live_in_[block];
//     auto &f_live_in = f_live_in_[block];
//     auto &live_out = live_out_[block];
//     auto &f_live_out = f_live_out_[block];

//     auto stmts = block->GetStmts();

//     // 先处理phi函数
//     for (auto &stmt: stmts) {
//       auto phi_ir = std::dynamic_pointer_cast<PhiIR>(stmt);
//       if (!phi_ir) {
//         continue;
//       }
//       auto oprnds = phi_ir->GetOprnds();
//       for (size_t i = 0; i < oprnds.size(); i += 2) {
//         auto var = oprnds[i]->GetResultVar();
//         if (oprnds[i]->GetResultVar()->IsImm()) {
//           continue;  // 立即数不能加入到livein和liveout中
//         }
//         auto block_decl_ir = std::dynamic_pointer_cast<BlockDeclIR>(oprnds[i + 1]);
//         auto parent_blk = block_decl_ir->blk_;
//         auto &parent_live_out = live_out_[parent_blk];
//         auto &parent_f_live_out = f_live_out_[parent_blk];
//         if (var->IsIntReg()) {
//           parent_live_out.insert(var);
//         } else if (var->IsFloatReg()) {
//           parent_f_live_out.insert(var);
//         }
//       }

//       auto d = stmt->GetResultVar();
//       if (d) {
//         if (d->IsFloatReg()) {
//           f_live_in.insert(d);
//         } else if (d->IsIntReg()) {
//           live_in.insert(d);
//         }
//       }
//     }
//     // 然后处理其他指令
//     for (auto stmt: stmts) {
//       if (std::dynamic_pointer_cast<PhiIR>(stmt)) {
//         continue;
//       }
//       for (auto &u: stmt->GetOperands()) {
//         if (!u->IsImm() && u->IsFloatReg() && !u->IsGbl()) {
//           if (f_defs.find(u) == f_defs.end()) {
//             f_uses.insert(u);
//           }
//         } else if (!u->IsImm() && u->IsIntReg() && !u->IsGbl()) {
//           if (defs.find(u) == defs.end()) {
//             uses.insert(u);
//           }
//         }
//       }
//       auto d = stmt->GetResultVar();
//       if (d) {
//         if (d->IsFloatReg()) {
//           f_defs.insert(d);
//         } else if (d->IsIntReg()) {
//           defs.insert(d);
//         }
//       }
//     }
//   }
// }

auto LiveAnalyzer::CalDefsUses() -> void {
  for (auto &block: cfg_->blocks_) {
    if (block->GetSequenceID() == 2) {
      int xx = 1;
    }
    auto &uses = uses_[block];
    auto &defs = defs_[block];
    auto &f_uses = f_uses_[block];
    auto &f_defs = f_defs_[block];
    for (auto &stmt: block->GetStmts()) {
      for (auto &u: stmt->GetOperands()) {
        if (!u->IsImm() && u->IsFloatReg() && !u->IsGbl()) {
          if (f_defs.find(u) == f_defs.end()){
            f_uses.insert(u);
          }
        } else if(!u->IsImm() && u->IsIntReg() && !u->IsGbl()) {
          if (defs.find(u) == defs.end()) {
            uses.insert(u);
          }
        }
      }
      auto d = stmt->GetResultVar();
      if (d) {
        if (d->IsFloatReg()) {
          f_defs.insert(d);
        } else if (d->IsIntReg()) {
          defs.insert(d);
        }
      }
    }
  }
}

auto LiveAnalyzer::CalLiveInOut() -> void {
  bool changed = false;
  auto blocks = cfg_->DFSTopology();

  do {
    changed = false;
    for (auto &blk: blocks) {
      auto old_live_out = live_out_[blk];
      live_out_[blk].clear();
      for (auto succ: blk->GetChildren()) {
        live_out_[blk].insert(live_in_[succ].begin(), live_in_[succ].end());
      }
      if (live_out_[blk] != old_live_out) {
        changed = true;
      }
      auto old_live_in = live_in_[blk];
      auto tmp(uses_[blk]);
      std::set_difference(live_out_[blk].begin(), live_out_[blk].end(),
                          defs_[blk].begin(), defs_[blk].end(),
                          std::inserter(tmp, tmp.begin()));
      live_in_[blk] = std::move(tmp);
    }
  } while (changed);

  // 处理浮点数
  do {
    changed = false;
    for (auto &blk: blocks) {
      auto old_f_live_out = f_live_out_[blk];
      f_live_out_[blk].clear();
      for (auto &succ: blk->GetChildren()) {
        f_live_out_[blk].insert(f_live_in_[succ].begin(), f_live_in_[succ].end());
      }
      if (f_live_out_[blk] != old_f_live_out) {
        changed = true;
      }
      auto old_f_live_in = f_live_in_[blk];
      auto tmp(f_uses_[blk]);
      std::set_difference(f_live_out_[blk].begin(), f_live_out_[blk].end(),
                          f_defs_[blk].begin(), f_defs_[blk].end(),
                          std::inserter(tmp, tmp.begin()));
      f_live_in_[blk] = std::move(tmp);
    }
  } while (changed);
}

auto LiveAnalyzer::Analyze() -> std::pair<std::map<IRVariablePtr, Range>, std::map<IRVariablePtr, Range>> {
  CalDefsUses();
  CalLiveInOut();
  auto blocks = cfg_->DFSTopology();
  // std::cout << "Analyze of " << cfg_->func_name_ << std::endl;

  for (auto &blk: blocks) {
    // std::cout << blk->GetStmts()[0]->GetResultVar()->name_ << " live_def: ";
    // for (auto &var: defs_[blk]) {
    //   std::cout << var->name_ << " ";
    // }
    // std::cout << std::endl;
    // std::cout << blk->GetStmts()[0]->GetResultVar()->name_ << " live_use: ";
    // for (auto &var: uses_[blk]) {
    //   std::cout << var->name_ << " ";
    // }
    // std::cout << std::endl;
    // std::cout << blk->GetStmts()[0]->GetResultVar()->name_ << " live_in: ";
    // for (auto &var: live_in_[blk]) {
    //   std::cout << var->name_ << " ";
    // }
    // std::cout << std::endl;
    // std::cout << blk->GetStmts()[0]->GetResultVar()->name_ << " live_out: ";
    // for (auto &var: live_out_[blk]) {
    //   std::cout << var->name_ << " ";
    // }
    // std::cout << std::endl;
    // 初始化live out中的变量
    for (auto &var: live_out_[blk]) {
      var2ranges_[var].insert({blk->begin_numbering_, blk->end_numbering_});
    }
    auto stmts = blk->GetStmts();
    std::reverse(stmts.begin(), stmts.end());
    for (auto stmt: stmts) {
      auto def = stmt->GetResultVar();
      if (def && def->IsIntReg()) {
        auto &live_range = var2ranges_[def];
        live_range.insert({stmt->numbering, stmt->numbering});

        for (auto range_it = live_range.begin(); range_it != live_range.end();) {
          if (stmt->numbering != blk->begin_numbering_ && range_it->first == blk->begin_numbering_) {
            // 截断活跃区间
            live_range.insert({stmt->numbering, range_it->second});
            live_range.erase(range_it++);
          } else {
            ++range_it;
          }
        }
      }

      for (auto &use: stmt->GetOperands()) {
        if (!use->IsImm() && use->IsIntReg() && !use->IsGbl()) {
          auto &live_range = var2ranges_[use];
          live_range.insert({blk->begin_numbering_, stmt->numbering});
        }
      }
    }
  }

  for (auto &blk: blocks) {
    // 初始化live out中的变量
    for (auto &var: f_live_out_[blk]) {
      var2ranges_[var].insert({blk->begin_numbering_, blk->end_numbering_});
    }
    auto stmts = blk->GetStmts();
    std::reverse(stmts.begin(), stmts.end());
    for (auto stmt: stmts) {
      auto def = stmt->GetResultVar();
      if (def && def->IsFloatReg()) {
        auto &live_range = f_var2ranges_[def];
        live_range.insert({stmt->numbering, stmt->numbering});

        for (auto range_it = live_range.begin(); range_it != live_range.end();) {
          if (stmt->numbering != blk->begin_numbering_ && range_it->first == blk->begin_numbering_) {
            // 截断活跃区间
            live_range.insert({stmt->numbering, range_it->second});
            live_range.erase(range_it++);
          } else {
            ++range_it;
          }
        }
      }

      for (auto &use: stmt->GetOperands()) {
        if (!use->IsImm() && use->IsFloatReg()) {
          auto &live_range = f_var2ranges_[use];
          live_range.insert({blk->begin_numbering_, stmt->numbering});
        }
      }
    }
  }

  std::map<std::shared_ptr<IRVariable>, Range> live_interval;
  std::map<std::shared_ptr<IRVariable>, Range> f_live_interval;
  for (auto &[var, ranges]: var2ranges_) {
    Range interval = *ranges.begin();
    for (auto &range: ranges) {
      interval.first = std::min(interval.first, range.first);
      interval.second = std::max(interval.second, range.second);
    }
    live_interval[var] = interval;
  }
  for (auto &[var, ranges]: f_var2ranges_) {
    Range interval = *ranges.begin();
    for (auto &range: ranges) {
      interval.first = std::min(interval.first, range.first);
      interval.second = std::max(interval.second, range.second);
    }
    f_live_interval[var] = interval;
  }

  return {live_interval, f_live_interval};
}


} // namespace SCNUCC