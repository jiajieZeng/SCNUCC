#include "../include/MyVisitor.h"
#include <iomanip>


namespace SCNUCC {

auto MyVisitor::Adjust() -> void {
  gbl_init_blk->AdjustID();
  for (auto &cfg: cfgs_) {
    cfg->AdjustID();
  }
  gbl_init_blk->SetDefUse();
  for (auto &cfg: cfgs_) {
    cfg->SetDefUse();
  }
}

auto MyVisitor::ToSSA() const -> std::string {
  std::stringstream ssa;
  /* 全局变量声明和初始化函数 */

  // 声明字符串池
  if (Util::CUR_POOLID > 1) {
    for (int i = 1; i < Util::CUR_POOLID; i++) {
      std::string s = Util::gstrings[Util::Poolid2Callid[i]];

      ssa << "@.str.";
      ssa << i;
      ssa << " = private unnamed_addr constant [";
      ssa << s.size() + 1;
      ssa << " x i8] c\"";
      ssa << s;
      ssa << "\\00\" align 1\n";
    }
    ssa << "\n\n";
  }

  if (!gbl_vars.empty()) {
    for (auto &item: gbl_vars) {
      auto gbl_decl = item.second;
      ssa << gbl_decl->ToSSA() << "\n";
    }
    // ssa << gbl_init_blk->ToSSA() << "\n";
    // ssa << "@llvm.global_ctors = appending global [1 x { i32, ptr, ptr }]
    // [{ i32, ptr, ptr } "
    //        "{ i32 65535, ptr @_GLOBAL__sub_I_example.cpp, ptr null }]\n";
    // for (auto &gbl_init_blk: gbl_init_blks) {
    //   ssa << gbl_init_blk->ToSSA() << "\n";
    // }
    // ssa << "define internal void @_GLOBAL__sub_I_example.cpp() #1 section
    // \".text.startup\" {\n"; for (auto &gbl_init_blk: gbl_init_blks) {
    //   ssa << "  call void @init_" << gbl_init_blk->GetVar() << "()\n";
    // }
    // ssa << "  ret void\n";
    // ssa << "}\n\n";
  }

  /* 输出CFG */
  for (const auto &cfg: cfgs_) {
    ssa << cfg->ToSSA();
  }

  ssa << "\n\ndeclare i32 @getint()" << std::endl;
  ssa << "declare i32 @getch()" << std::endl;
  ssa << "declare float @getfloat()" << std::endl;
  ssa << "declare i32 @getarray(i32* %0)" << std::endl;
  ssa << "declare i32 @getfarray(float* %0)" << std::endl;
  ssa << "declare void @putint(i32 %0)" << std::endl;
  ssa << "declare void @putch(i32 %0)" << std::endl;
  ssa << "declare void @putarray(i32 %0, i32* %1)" << std::endl;
  ssa << "declare void @putfloat(float %0)" << std::endl;
  ssa << "declare void @putfarray(i32 %0, float* %1)" << std::endl;
  // ssa << "declare void @putf(i32* %0, ...)" << std::endl;
  ssa << "declare void @putf(i8* %0, ...)" << std::endl;
  ssa << "declare void @before_main()" << std::endl;
  ssa << "declare void @after_main()" << std::endl;
  ssa << "declare void @_sysy_starttime(i32 %0)" << std::endl;
  ssa << "declare void @_sysy_stoptime(i32 %0)" << std::endl;
  ssa << "\n\ndeclare void @llvm.memset.p0.i32(ptr nocapture writeonly, i8, "
         "i32, i1 immarg)";
  return ssa.str();
}

MyVisitor::MyVisitor() {
  sym_table_ = std::make_shared<SymbolTable>();
  SyLibInit();
  gbl_init_blk = std::make_shared<GblInitBlock>();
  return_flag_ = BLOCK::NO_RETURN;
}

std::any MyVisitor::visitCompUnit(SysYParser::CompUnitContext *ctx) {
  for (auto child: ctx->children) {
    visit(child);
  }
  return nullptr;
}

std::any MyVisitor::visitDecl(SysYParser::DeclContext *ctx) {
  if (auto constDecl = ctx->constDecl()) {
    visit(constDecl);
  } else if (auto varDecl = ctx->varDecl()) {
    visit(varDecl);
  }
  return nullptr;
}

std::any MyVisitor::visitConstDecl(SysYParser::ConstDeclContext *ctx) {
  for (auto constDef: ctx->constDef()) {
    // 每一条语句都是一个全局变量的声明语句
    visit(constDef);
  }
  return nullptr;
}

std::any MyVisitor::visitConstDef(SysYParser::ConstDefContext *ctx) {
  auto parent = dynamic_cast<SysYParser::ConstDeclContext *>(ctx->parent);
  auto var_type = toBaseType(parent->bType()->getText());
  std::string var_name = ctx->ident()->getText();
  if (var_name.size() > 31) {
    var_name = var_name.substr(0,5) + "_";
  }
  // 看看当前作用域有没有定义
  if (sym_table_->VarIsDefined(var_name, SearchScope::CUR_SCOPE)) {
    throw std::runtime_error("redefinition of variable " + var_name);
  }
  Type type(var_type, SymKind::VAR, true, sym_table_->IsGlobalScope());
  auto var_symbol = std::make_shared<VarSymbol>(type, var_name);
  if (sym_table_->IsGlobalScope()) {  // 全局变量，生成一个用于声明全局变量的GblVarDeclIR
    // 变量
    if (ctx->constExp().empty()) {
      sym_table_->Insert(var_symbol);
      // 根据sysY的语法定义，const的定义一定存在初始化值。生成一个GblInitBlock，用于计算初始化值
      auto init_ir = std::any_cast<IRPtr>(visit(ctx->constInitVal()));
      if (!IsConst(init_ir->GetIRType())) {
        throw std::runtime_error("Error: constInitVal's return value is not a constant");
      }
      LoadVar(init_ir);
      if (var_type == BaseType::INT) {
        ZeroExtend(init_ir);
        Float2Int(init_ir);
        int value = std::any_cast<int>(init_ir->GetConstantVal());
        var_symbol->SetScalerValue(value);
      } else if (var_type == BaseType::FLOAT) {
        Int2Float(init_ir);
        float value = std::any_cast<float>(init_ir->GetConstantVal());
        var_symbol->SetScalerValue(value);
      }
      // auto gbl_decl_stmt = std::make_shared<GblVarDeclIR>(toIRType(var_type), var_symbol);
      auto gbl_decl_stmt = GblVarDeclIR::Create(toIRType(var_type), var_symbol);
      gbl_vars.insert({var_name, gbl_decl_stmt});
      gbl_init_blk->AddStatement(gbl_decl_stmt);

    } else {
      // 数组
      // 提取每一个维度的长度
      size_t num = ctx->constExp().size();
      for (size_t i = 0; i < num; i++) {
        auto shape_ir = std::any_cast<IRPtr>(visit(ctx->constExp(i)));
        size_t shape = (size_t) std::any_cast<int>(shape_ir->GetConstantVal());
        type.array_shape_.emplace_back(shape);
      }
      auto var_symbol = std::make_shared<VarSymbol>(type, var_name);
      sym_table_->Insert(var_symbol);
      // 存在初始化的值
      auto array_value = GetArrayInitVal(0, var_symbol, ctx->constInitVal());
      std::vector<size_t> indices;
      if (type.base_type_ == BaseType::INT) {
        std::map<int, int> mp;
        DumpConstArray<int>(var_symbol, indices, array_value, mp);
        var_symbol->SetValue(mp);
        var_symbol->SetArrayVal(array_value);
      } else {
        std::map<int, float> mp;
        DumpConstArray<float>(var_symbol, indices, array_value, mp);
        var_symbol->SetValue(mp);
        var_symbol->SetArrayVal(array_value);
      }
      // 这个是global的，所以不需要memset，需要zeroinit
      // 通过gbl var decl进行初始化
      // auto gbl_decl_stmt = std::make_shared<GblVarDeclIR>(var_type == BaseType::INT ? IRTYPE::IR_INT_PTR : IRTYPE::IR_FLOAT_PTR, var_symbol);
      auto gbl_decl_stmt = GblVarDeclIR::Create(var_type == BaseType::INT ? IRTYPE::IR_INT_PTR : IRTYPE::IR_FLOAT_PTR,
                                                var_symbol);
      gbl_vars.insert({var_name, gbl_decl_stmt});
      // 初始化
      std::stringstream suffix;
      GlobalInitArray(var_symbol, array_value, 0, suffix);
      gbl_decl_stmt->SetSuffix(suffix.str());
      gbl_decl_stmt->SetValues(array_value);
      gbl_init_blk->AddStatement(gbl_decl_stmt);
    }
    return nullptr;
  } else {  // 局部变量，返回一系列用于定义局部变量的IRStatement
    auto cur_blk = cfgs_.back()->GetLastBlock();
    // 变量
    if (ctx->constExp().empty()) {
      sym_table_->Insert(var_symbol);
      // 根据sysY的语法定义，const的定义一定存在初始化值
      auto init_stmt = std::any_cast<IRPtr>(visit(ctx->constInitVal()));
      if (!IsConst(init_stmt->GetIRType())) {
        throw std::runtime_error("Error: constInitVal's return value is not a constant");
      }
      if (type.base_type_ == BaseType::INT) {
        ZeroExtend(init_stmt);
        Float2Int(init_stmt);
      } else {
        Int2Float(init_stmt);
      }
      if (init_stmt->GetIRType() == IRTYPE::IR_CONST_INT) {
        int val_num = std::any_cast<int>(init_stmt->GetConstantVal());
        var_symbol->SetScalerValue(val_num);
      } else {
        float val_num = std::any_cast<float>(init_stmt->GetConstantVal());
        var_symbol->SetScalerValue(val_num);
      }
      WriteVariable(var_name, init_stmt);
    } else {
      // 数组
      // 提取每一个维度的长度
      size_t num = ctx->constExp().size();
      for (size_t i = 0; i < num; i++) {
        auto shape_ir = std::any_cast<IRPtr>(visit(ctx->constExp(i)));
        size_t shape = (size_t) std::any_cast<int>(shape_ir->GetConstantVal());
        type.array_shape_.emplace_back(shape);
      }
      auto var_symbol = std::make_shared<VarSymbol>(type, var_name);
      sym_table_->Insert(var_symbol);
      // IRPtr array_decl = std::make_shared<ArrayDeclIR>(var_type == BaseType::INT ? IRTYPE::IR_INT_PTR : IRTYPE::IR_FLOAT_PTR, var_symbol);
      IRPtr array_decl = ArrayDeclIR::Create(var_type == BaseType::INT ? IRTYPE::IR_INT_PTR : IRTYPE::IR_FLOAT_PTR,
                                             var_symbol);
      cur_blk->AddStatement(array_decl);
      // 存在初始化的值
      auto array_value = GetArrayInitVal(0, var_symbol, ctx->constInitVal());
      IRStatementVec indices;
      std::vector<size_t> const_indices;
      if (type.base_type_ == BaseType::INT) {
        std::map<int, int> mp;
        DumpConstArray<int>(var_symbol, const_indices, array_value, mp);
        var_symbol->SetValue(mp);
        var_symbol->SetArrayVal(array_value);
      } else {
        std::map<int, float> mp;
        DumpConstArray<float>(var_symbol, const_indices, array_value, mp);
        var_symbol->SetValue(mp);
        var_symbol->SetArrayVal(array_value);
      }
      // 这个不是global的，需要用memset
      // 先来一个bitcast
      // auto bitcast = std::make_shared<BitCastIR>(type.base_type_ == BaseType::INT ? IRTYPE::IR_I32 : IRTYPE::IR_FLOAT, var_symbol);
      auto bitcast = BitCastIR::Create(type.base_type_ == BaseType::INT ? IRTYPE::IR_I32 : IRTYPE::IR_FLOAT,
                                       var_symbol);
      bitcast->AddOprnd(array_decl);
      cur_blk->AddStatement(bitcast);
      // 然后再慢慢去调用GEP慢慢初始化
      indices.clear();
      LocalInitArray(var_symbol, array_decl, indices, array_value);
      WriteVariable(var_name, array_decl);
    }
    return nullptr;
  }
}

// constInitVal是可计算出来的
std::any MyVisitor::visitConstInitVal(SysYParser::ConstInitValContext *ctx) {
  // TODO: 考虑数组的情况
  // 改掉这个
  if (ctx->LBRACE() == nullptr) {
    auto ir = std::any_cast<IRPtr>(visit(ctx->constExp()));
    return ir;
  }
  return nullptr;
}

std::any MyVisitor::visitVarDecl(SysYParser::VarDeclContext *ctx) {
  for (auto varDef: ctx->varDef()) {
    visit(varDef);
  }
  return nullptr;
}

std::any MyVisitor::visitVarDef(SysYParser::VarDefContext *ctx) {
  auto parent = dynamic_cast<SysYParser::VarDeclContext *>(ctx->parent);
  auto var_type = toBaseType(parent->bType()->getText());
  std::string var_name = ctx->ident()->getText();
  if (var_name.size() > 31) {
    var_name = var_name.substr(0,5) + "_";
  }
  Type type(var_type, SymKind::VAR, false, sym_table_->IsGlobalScope());
  // 看看当前作用域有没有定义
  if (sym_table_->VarIsDefined(var_name, SearchScope::CUR_SCOPE)) {
    throw std::runtime_error("redefinition of variable " + var_name);
  }
  if (sym_table_->IsGlobalScope()) {
    if (ctx->constExp().empty()) {  // 变量
      auto var_symbol = std::make_shared<VarSymbol>(type, var_name);
      sym_table_->Insert(var_symbol);
      // 如果有初始化值，那么就返回计算初始化值的IRStatement
      if (ctx->initVal() != nullptr) {
        // init_ir已经在visitExp中放入到blk中了
        auto init_ir = std::any_cast<IRPtr>(visit(ctx->initVal()));
        if (var_type == BaseType::INT) {
          ZeroExtend(init_ir);
          Float2Int(init_ir);
          int value = std::any_cast<int>(init_ir->GetConstantVal());
          var_symbol->SetScalerValue(value);
        } else if (var_type == BaseType::FLOAT) {
          Int2Float(init_ir);
          std::map<int, float> values;
          values[0] = std::any_cast<float>(init_ir->GetConstantVal());
          var_symbol->SetValue(values);
        }
      } else {
        if (var_type == BaseType::INT) {
          var_symbol->SetScalerValue(int(0));
        } else {
          var_symbol->SetScalerValue(0.0);
        }
      }
      // auto gbl_dcl_stmt = std::make_shared<GblVarDeclIR>(toIRType(var_type), var_symbol);
      auto gbl_dcl_stmt = GblVarDeclIR::Create(toIRType(var_type), var_symbol);
      gbl_vars.insert({var_name, gbl_dcl_stmt});
      gbl_init_blk->AddStatement(gbl_dcl_stmt);
    } else {
      // 数组
      // 提取每一个维度的长度
      size_t num = ctx->constExp().size();
      for (size_t i = 0; i < num; i++) {
        auto shape_ir = std::any_cast<IRPtr>(visit(ctx->constExp(i)));
        size_t shape = (size_t) std::any_cast<int>(shape_ir->GetConstantVal());
        type.array_shape_.emplace_back(shape);
      }
      auto var_symbol = std::make_shared<VarSymbol>(type, var_name);
      sym_table_->Insert(var_symbol);
      // auto gbl_decl_stmt = std::make_shared<GblVarDeclIR>(var_type == BaseType::INT ? IRTYPE::IR_INT_PTR : IRTYPE::IR_FLOAT_PTR, var_symbol);
      std::shared_ptr<GblVarDeclIR> gbl_decl_stmt = GblVarDeclIR::Create(
              var_type == BaseType::INT ? IRTYPE::IR_INT_PTR : IRTYPE::IR_FLOAT_PTR, var_symbol);
      gbl_vars.insert({var_name, gbl_decl_stmt});
      // 这个是global的，所以不需要memset，需要zeroinit
      // 通过gbl var decl进行初始化
      // 存在初始化的值
      if (ctx->initVal() != nullptr && ctx->initVal()->getText() != "{}") {
        auto array_value = GetArrayInitVal(0, var_symbol, ctx->initVal());
        std::vector<size_t> indices;
        if (type.base_type_ == BaseType::INT) {
          std::map<int, int> mp;
          DumpConstArray<int>(var_symbol, indices, array_value, mp);
          var_symbol->SetValue(mp);
          var_symbol->SetArrayVal(array_value);
        } else {
          std::map<int, float> mp;
          DumpConstArray<float>(var_symbol, indices, array_value, mp);
          var_symbol->SetValue(mp);
          var_symbol->SetArrayVal(array_value);
        }
        // 初始化
        std::stringstream suffix;
        GlobalInitArray(var_symbol, array_value, 0, suffix);
        gbl_decl_stmt->SetSuffix(suffix.str());
        gbl_decl_stmt->SetValues(array_value);
      } else {
        std::ostringstream suffix;
        // std::string suffix = "zeroinitializer";
        std::vector<size_t> vt = type.array_shape_;
        for (size_t i = 0; i < vt.size(); i++) {
          suffix << "[" << vt[i] << " x";
        }
        if (type.base_type_ == BaseType::INT) {
          suffix << " i32";
        } else {
          suffix << " float";
        }
        for (size_t i = 0; i < vt.size(); i++) {
          suffix << "]";
        }
        suffix << " zeroinitializer";
        gbl_decl_stmt->SetSuffix(suffix.str());
      }
      gbl_init_blk->AddStatement(gbl_decl_stmt);
    }
  } else {
    auto cur_blk = cfgs_.back()->GetLastBlock();
    if (ctx->constExp().empty()) {
      auto var_symbol = std::make_shared<VarSymbol>(type, var_name);
      sym_table_->Insert(var_symbol);
      // 变量
      if (ctx->initVal() != nullptr) {
        // 如果有初始化值，那么就返回计算初始化值的IRStatement
        auto init_stmt = std::any_cast<IRPtr>(visit(ctx->initVal()));
        LoadVar(init_stmt);
        if (type.base_type_ == BaseType::INT) {
          ZeroExtend(init_stmt);
          Float2Int(init_stmt);
        } else {
          Int2Float(init_stmt);
        }
        WriteVariable(var_name, init_stmt);
      } else {
        // TODO：更准确的做法是，不生成任何的语句，因为这个变量还没被使用，所以不需要分配一个value
        std::shared_ptr<ConstantIR> ir_stmt;
        if (var_type == BaseType::INT) {
          // ir_stmt = std::make_shared<ConstantIR>(0, IRTYPE::IR_CONST_INT);
          ir_stmt = ConstantIR::Create(0, IRTYPE::IR_CONST_INT);
        } else if (var_type == BaseType::FLOAT) {
          // ir_stmt = std::make_shared<ConstantIR>(0, IRTYPE::IR_CONST_FLOAT);
          ir_stmt = ConstantIR::Create(0, IRTYPE::IR_CONST_FLOAT);
        }
        WriteVariable(var_name, ir_stmt);
      }
    } else {
      // 数组
      // 提取每一个维度的长度
      size_t num = ctx->constExp().size();
      for (size_t i = 0; i < num; i++) {
        auto shape_ir = std::any_cast<IRPtr>(visit(ctx->constExp(i)));
        size_t shape = (size_t) std::any_cast<int>(shape_ir->GetConstantVal());
        type.array_shape_.emplace_back(shape);
      }
      auto var_symbol = std::make_shared<VarSymbol>(type, var_name);
      sym_table_->Insert(var_symbol);
      // IRPtr array_decl = std::make_shared<ArrayDeclIR>(var_type == BaseType::INT ? IRTYPE::IR_INT_PTR : IRTYPE::IR_FLOAT_PTR, var_symbol);
      IRPtr array_decl = ArrayDeclIR::Create(var_type == BaseType::INT ? IRTYPE::IR_INT_PTR : IRTYPE::IR_FLOAT_PTR,
                                             var_symbol);
      cur_blk->AddStatement(array_decl);
      if (ctx->initVal() != nullptr) {
        // 存在初始化的值
        auto array_value = GetArrayInitVal(0, var_symbol, ctx->initVal());
        IRStatementVec indices;
        // 这个不是global的，需要用memset
        // 先来一个bitcast
        // auto bitcast = std::make_shared<BitCastIR>(type.base_type_ == BaseType::INT ? IRTYPE::IR_I32 : IRTYPE::IR_FLOAT, var_symbol);
        auto bitcast = BitCastIR::Create(type.base_type_ == BaseType::INT ? IRTYPE::IR_I32 : IRTYPE::IR_FLOAT,
                                         var_symbol);
        bitcast->AddOprnd(array_decl);
        cur_blk->AddStatement(bitcast);
        // 然后再慢慢去调用GEP慢慢初始化
        indices.clear();
        LocalInitArray(var_symbol, array_decl, indices, array_value);
      }
      WriteVariable(var_name, array_decl);
    }

  }
  return nullptr;
}

std::any MyVisitor::visitInitVal(SysYParser::InitValContext *ctx) {
  auto ir = std::any_cast<IRPtr>(visit(ctx->exp()));
  LoadVar(ir);
  return ir;
}

std::any MyVisitor::visitFuncDef(SysYParser::FuncDefContext *ctx) {
  std::string func_name = ctx->ident()->getText();
  if (func_name.size() > 31) {
    func_name = func_name.substr(0,5) + "_";
  }
  if (sym_table_->FuncLookUp(func_name)) {
    throw std::runtime_error("redefinition of function " + func_name);
  }
  auto func_symbol = buildFuncSymbol(ctx);
  sym_table_->Insert(func_symbol);
  // 重新初始化一些静态变量
  Block::block_cnt_ = 0;
  IRStatement::def_cnt_ = 0;
  return_flag_ = BLOCK::NO_RETURN;
  // 创建一个新的cfg
  cfgs_.push_back(std::make_shared<ControlFlowGraph>(sym_table_, func_name));
  auto blk =  Block::Create(sym_table_, nullptr, nullptr);
  blk->Seal();                // blk没有父节点，所以是sealed的
  cfgs_.back()->Concat(blk);  // 加入第一个block
  // 这个block有没有return
  return_flag_ = BLOCK::NO_RETURN;
  // 单出口原则，一定要构建ret block
  // 构建最后的return_block_
  // 中间所有的ret都用br跳到这里
  return_block_ =  Block::Create(sym_table_, nullptr, nullptr);

  visit(ctx->block());

  std::string return_type = ctx->funcType()->getText();

  auto last_block = cfgs_.back()->GetLastBlock();
  auto stmt = last_block->GetStmts().back();
  int flag = 1; // flag == 1表示不需要添加br
  if (!std::dynamic_pointer_cast<RetIR>(stmt) && !std::dynamic_pointer_cast<BRIR>(stmt)) {
    flag = 0; // 需要添加br
  }

  if (return_type == "void") {
    // auto ret = std::make_shared<RetIR>(IRTYPE::IR_VOID);
    auto ret = RetIR::Create(IRTYPE::IR_VOID);
    return_block_->AddStatement(ret);
    if (!flag) {
      // auto br = std::make_shared<BRIR>(IRTYPE::IR_I1, last_block);
      auto br = BRIR::Create(IRTYPE::IR_I1, last_block);
      br->AddBranch(return_block_);
      last_block->AddStatement(br);
    }
    cfgs_.back()->Concat(return_block_);
  } else {
    // ret最后才放，可能需要放phi语句
    auto ret_type = return_type == "int" ? IRTYPE::IR_I32 : IRTYPE::IR_FLOAT;
    // auto ret_ir = std::make_shared<RetIR>(ret_type);
    auto ret_ir = RetIR::Create(ret_type);
    auto stmts = return_block_->GetStmts();
    cfgs_.back()->Concat(return_block_);  // 连接着两个block

    if (stmts.size() == 1) {  // 就是说有返回值的函数，忘了写return， 那就默认返回一个0
      // auto zero_ir = std::make_shared<ConstantIR>(0, ret_type == IRTYPE::IR_I32 ? IRTYPE::IR_CONST_INT : IRTYPE::IR_CONST_FLOAT);
      auto zero_ir = ConstantIR::Create(0, ret_type == IRTYPE::IR_I32 ? IRTYPE::IR_CONST_INT : IRTYPE::IR_CONST_FLOAT);
      ret_ir->AddOprnd(zero_ir);
    } else {
      // 0号是blockDeclear，1号肯定是phi
      // 这个phi语句，需要扫描return_block_的所有parent节点
      // 如果没有来自该parent节点的操作数，那么就此时需要返回未定义的值，建议返回0
      auto phi = std::dynamic_pointer_cast<PhiIR>(stmts[1]);
      return_block_->AddReturnVal(phi, ret_type == IRTYPE::IR_I32 ? IRTYPE::IR_CONST_INT : IRTYPE::IR_CONST_FLOAT);
      ret_ir->AddOprnd(phi);
      // return_block_->TryRemoveTrivialPhi(phi);
    }
    if (!flag) {
      // auto br = std::make_shared<BRIR>(IRTYPE::IR_I1, last_block);
      auto br = BRIR::Create(IRTYPE::IR_I1, last_block);
      br->AddBranch(return_block_);
      last_block->AddStatement(br);
    }
    return_block_->AddStatement(ret_ir);

  }

  return nullptr;
}

auto MyVisitor::fillFuncParam(SysYParser::FuncDefContext *ctx) -> void {
  auto func_name = ctx->ident()->getText();
  if (func_name.size() > 31) {
    func_name = func_name.substr(0,5) + "_";
  }
  auto func_symbol = sym_table_->FuncLookUp(func_name);
  // 如果函数有参数，遍历每一个funcParam孩子节点，获取参数类型
  size_t param_sequence_id = 0;
  if (ctx->funcFParams() != nullptr) {
    std::vector<Type> param_types;
    for (auto param: ctx->funcFParams()->funcFParam()) {
      auto param_sym = std::any_cast<std::shared_ptr<VarSymbol>>(visit(param));
      sym_table_->Insert(param_sym);
      IRTYPE::IRTYPE type;
      if (param_sym->GetType().base_type_ == BaseType::INT) {
        if (param->LBRACK().size()) {
          type = IRTYPE::IR_INT_PTR;
        } else {
          type = IRTYPE::IR_I32;
        }
      } else {
        if (param->LBRACK().size()) {
          type = IRTYPE::IR_FLOAT_PTR;
        } else {
          type = IRTYPE::IR_FLOAT;
        }
      }
      // auto param_ir = std::make_shared<FuncParamIR>(type);
      auto param_ir = FuncParamIR::Create(type, (int)param_sequence_id);
      cfgs_.back()->AddParam(param_ir);  // 添加到函数的参数列表中
      param_ir->SetSequenceID(param_sequence_id);
      ++param_sequence_id;
      std::string param_name = param_sym->GetString();
      WriteVariable(param_name, param_ir); // 将这个变量的值写入局部值编码的映射表
      param_types.push_back(param_sym->GetType());
    }
    func_symbol->SetAllParams(param_types);  // 设置函数参数类型
  }
}

std::any MyVisitor::visitFuncType(SysYParser::FuncTypeContext *ctx) {
  // 不应该调用这个函数
  throw std::runtime_error("visitFuncFParams should not be called");
}

std::any MyVisitor::visitFuncFParams(SysYParser::FuncFParamsContext *ctx) {
  // 不应该调用这个函数
  throw std::runtime_error("visitFuncFParams should not be called");
}

std::any MyVisitor::visitFuncFParam(SysYParser::FuncFParamContext *ctx) {
  auto var_type = toBaseType(ctx->bType()->getText());
  std::string var_name = ctx->ident()->getText();
  if (var_name.size() > 31) {
    var_name = var_name.substr(0,5) + "_";
  }
  Type type(var_type, SymKind::VAR, false, false);
  if (ctx->LBRACK().size()) {
    // 第一个是0，其余的都是数字
    type.array_shape_.push_back(0);
    for (auto exp: ctx->exp()) {
      auto ir = std::any_cast<IRPtr>(visit(exp));
      size_t num = std::any_cast<int>(ir->GetConstantVal());
      type.array_shape_.push_back(num);
    }
  }
  auto var_symbol = std::make_shared<VarSymbol>(type, var_name);
  return var_symbol;
}

std::any MyVisitor::visitBlock(SysYParser::BlockContext *ctx) {
  sym_table_->EnterScope();  // 进入一个新的作用域
  // 如果parent是函数定义
  if (dynamic_cast<SysYParser::FuncDefContext *>(ctx->parent) != nullptr) {
    auto func_def = dynamic_cast<SysYParser::FuncDefContext *>(ctx->parent);
    fillFuncParam(func_def);
  }
  auto cfg = cfgs_.back();  // 这个cfg是在进入block之前创建的

  for (auto item: ctx->blockItem()) {
    // 如果已经return 那么就不递归visit了
    if (return_flag_ != BLOCK::NO_RETURN) {
      break;
    }

    // child可能是decl或者是stmt
    if (auto decl = item->decl()) {
      visit(decl);
    } else if (auto stmt = item->stmt()) {
      visit(stmt);
    }
  }
  sym_table_->ExitScope();

  return nullptr;
}

std::any MyVisitor::visitBlockItem(SysYParser::BlockItemContext *ctx) {
  // 不应该调用这个函数
  throw std::runtime_error("visitBlockItem should not be called");
}

std::any MyVisitor::visitStmt(SysYParser::StmtContext *ctx) {
  if (auto lval = ctx->lVal()) { // Stmt -> LVal '=' Exp ';'
    auto var_name = lval->ident()->getText();
    if (var_name.size() > 31) {
      var_name = var_name.substr(0,5) + "_";
    }
    // 从当前作用域到父作用域都检查
    if (!sym_table_->VarIsDefined(var_name, SearchScope::ALL_SCOPE)) {
      throw std::runtime_error("undefined variable " + var_name);
    }
    // 现在这个作用域找
    std::shared_ptr<VarSymbol> var_symbol = std::dynamic_pointer_cast<VarSymbol>(
            sym_table_->VarLookUp(var_name, SearchScope::CUR_SCOPE));
    if (var_symbol == nullptr) {
      var_symbol = std::dynamic_pointer_cast<VarSymbol>(sym_table_->VarLookUp(var_name, SearchScope::ALL_SCOPE));
    }
    if (var_symbol->IsConst()) {
      throw std::runtime_error("cannot assign to const variable " + var_name);
    }
    auto blk = GetLastBlock();
    auto var_type = var_symbol->GetType().base_type_;
    auto exp_ir = std::any_cast<IRPtr>(visit(ctx->exp()));
    LoadVar(exp_ir);
    if (var_type == BaseType::INT) {
      ZeroExtend(exp_ir);
      Float2Int(exp_ir);
    } else if (var_type == BaseType::FLOAT) {
      Int2Float(exp_ir);
    }
    // 变量
    if (ctx->lVal()->LBRACK().empty()) {
      WriteVariable(var_name, exp_ir); // 将这个变量的值写入局部值编码的映射表
    } else {  // 数组
      // 最后一个exp是=右边的
      auto gep = std::any_cast<IRPtr>(visit(ctx->lVal()));  // 拿到这个gep
      // auto store = std::make_shared<StoreIR>(var_type == BaseType::INT ? IRTYPE::IR_I32 : IRTYPE::IR_FLOAT);
      auto store = StoreIR::Create(var_type == BaseType::INT ? IRTYPE::IR_I32 : IRTYPE::IR_FLOAT);
      store->AddOprnd(exp_ir);
      store->AddOprnd(gep);
      blk->AddStatement(store);
    }
  } else if (auto if_ = ctx->IF()) {  // Stmt -> 'if' '( Cond ')' Stmt [ 'else' Stmt ]
    std::shared_ptr<Block> pre_while_entry = nullptr, pre_while_body = nullptr, pre_while_exit = nullptr;
    if (while_list_.size()) {
      auto last_while = while_list_.back();
      pre_while_entry = std::get<0>(last_while);
      pre_while_body = std::get<1>(last_while);
      pre_while_exit = std::get<2>(last_while);
    }
    auto cfg = cfgs_.back();
    auto if_entry = cfgs_.back()->GetLastBlock();
    auto if_then =  Block::Create(sym_table_, pre_while_entry, pre_while_exit);
    std::shared_ptr<Block> if_else = nullptr;
    if (ctx->stmt().size() == 2) {
      if_else =  Block::Create(sym_table_, pre_while_entry, pre_while_exit);
    }
    auto if_end =  Block::Create(sym_table_, pre_while_entry, pre_while_exit);
    int connectToEnd = 0;
    // cfg->AddBlock(if_entry);
    // 访问条件表达式
    visit(ctx->cond());
    // 在back patch中，已经完成了if_then与if_entry的许多block的连接
    backPatch(true_list.back(), if_then, true);
    backPatch(false_list.back(), if_else == nullptr ? if_end : if_else, false);
    true_list.pop_back();
    false_list.pop_back();
    if_entry->Seal();

    // 构造then part
    cfg->AddBlock(if_then);
    if_then->Seal();  // if_then唯一的前驱已经找到，所以是sealed的
    BLOCK::BLOCK then_return, else_return;
    visit(ctx->stmt(0));
    bool blk_ret1 = false, blk_ret2 = false; // if-then和if-else是否有break、return、continue等语句
    // 添加br语句
    if (return_flag_ == BLOCK::NO_RETURN) {
      // 没有return, continue, break就跳到end
      // auto br2then = std::make_shared<BRIR>(IRTYPE::IR_I1, if_then);
      auto br2then = BRIR::Create(IRTYPE::IR_I1, if_then);
      br2then->AddBranch(if_end);
      cfg->GetLastBlock()->AddStatement(br2then);
      ControlFlowGraph::ConnectBlocks(cfg->GetLastBlock(), if_end);
      connectToEnd = 1;
    } else {
      // 有就不用管了
      return_flag_ = BLOCK::NO_RETURN;
      blk_ret1 = true;
    }

    // 构造else part
    if (if_else != nullptr) {
      cfg->AddBlock(if_else);
      if_else->Seal();  // if_else唯一的前驱已经找到，所以是sealed的
      visit(ctx->stmt(1));
      if (return_flag_ == BLOCK::NO_RETURN) {
        // 没有return, continue, break就跳到end
        // auto br2end = std::make_shared<BRIR>(IRTYPE::IR_I1, if_else);
        auto br2end = BRIR::Create(IRTYPE::IR_I1, if_else);
        br2end->AddBranch(if_end);
        cfg->GetLastBlock()->AddStatement(br2end);
        ControlFlowGraph::ConnectBlocks(cfg->GetLastBlock(), if_end);
        connectToEnd = 1;
      } else {
        // 有就不用管了
        return_flag_ = BLOCK::NO_RETURN;
        blk_ret2 = true;
      }
    }
    if (!blk_ret1 || !blk_ret2) {  // 如果if-then或if-else链接到了if-end，那就把if-end加入到cfg中
      cfg->AddBlock(if_end);
      if_end->Seal();  // if_end的两个前驱都找到了，所以是sealed的
      // if_end是Seal
    } else {
      return_flag_ = BLOCK::HAS_BREAK;
    }
    return nullptr;
  } else if (auto while_ = ctx->WHILE()) {  // Stmt -> 'while' '(' Cond ')' Stmt
    auto cfg = cfgs_.back();
    std::shared_ptr<Block> pre_while_entry = nullptr,
            pre_while_body = nullptr,
            pre_while_exit = nullptr;
    if (!while_list_.empty()) {
      auto last_while = while_list_.back();
      pre_while_entry = std::get<0>(last_while);
      pre_while_body = std::get<1>(last_while);
      pre_while_exit = std::get<2>(last_while);
    }
    // entry部分不能使用cfg的最后一个块，因为entry部分可能会有多个前驱，所以需要新建一个块
    auto while_entry =  Block::Create(sym_table_, pre_while_entry,
                                               pre_while_exit);
    auto while_body =  Block::Create(sym_table_, pre_while_entry,
                                              pre_while_exit);
    auto while_exit =  Block::Create(sym_table_, pre_while_entry,
                                              pre_while_exit);

    while_list_.emplace_back(while_entry, while_body, while_exit);

    // 构造Entry,entry需要跟前一个块连起来
    auto blk_before_entry = cfg->GetLastBlock();
    // auto br1 = std::make_shared<BRIR>(IRTYPE::IR_I1, blk_before_entry);
    auto br1 = BRIR::Create(IRTYPE::IR_I1, blk_before_entry);
    br1->AddBranch(while_entry);
    blk_before_entry->AddStatement(br1);
    cfg->Concat(while_entry);
    visit(ctx->cond());
    backPatch(true_list.back(), while_body, true);
    backPatch(false_list.back(), while_exit, false);
    true_list.pop_back();
    false_list.pop_back();
    // 到这里entry还没seal

    while_body->Seal();  // 到这里body的前驱已经完全
    // 构造Body
    cfg->AddBlock(while_body);  // 这里要先用add
    visit(ctx->stmt(0));
    if (return_flag_ == BLOCK::NO_RETURN) {  // 如果没return，没continue，没break
      // auto br2entry = std::make_shared<BRIR>(IRTYPE::IR_I1, cfg->GetLastBlock());
      auto br2entry = BRIR::Create(IRTYPE::IR_I1, cfg->GetLastBlock());
      br2entry->AddBranch(while_entry);
      cfg->GetLastBlock()->AddStatement(br2entry);
      // 这个body可以连向entry
      ControlFlowGraph::ConnectBlocks(cfg->GetLastBlock(), while_entry);
      cfg->AddBackEdge(while_body, while_entry);
    } else {
      // 下面的逻辑移到了对应的语句中
      // // 如果是continue，那么需要连接回去entry
      // if (return_flag_ == BLOCK::HAS_CONTINUE) {
      //   ControlFlowGraph::ConnectBlocks(cfg->GetLastBlock(),
      //   while_entry); auto br2entry =
      //   std::make_shared<BRIR>(IRTYPE::IR_I1, cfg->GetLastBlock());
      //   br2entry->AddBranch(while_entry);
      //   cfg->GetLastBlock()->AddStatement(br2entry);
      // } else if (return_flag_ == BLOCK::HAS_BREAK) { //
      // 有break，body连向end
      //   ControlFlowGraph::ConnectBlocks(cfg->GetLastBlock(),
      //   while_exit);
      // }
      return_flag_ = BLOCK::NO_RETURN;
    }
    while_entry->Seal();  // while_entry的前驱已经找到，所以是sealed的

    // 构造Exit
    //    ControlFlowGraph::ConnectBlocks(while_entry, while_exit);
    cfg->AddBlock(while_exit);
    while_exit->Seal();  // while_exit的前驱都已经找到了，所以是sealed的

    // 这个while已经结束了。pop掉
    while_list_.pop_back();
    return nullptr;

  } else if (auto break_ = ctx->BREAK()) {  // Stmt -> 'break' ';'
    if (while_list_.empty()) {            // 不在while循环
      throw std::runtime_error("break statement not within a loop");
    }
    auto blk = cfgs_.back()->GetLastBlock();
    // auto br = std::make_shared<BRIR>(IRTYPE::IR_I1, blk);
    auto br = BRIR::Create(IRTYPE::IR_I1, blk);
    auto while_exit = std::get<2>(while_list_.back());
    br->AddBranch(while_exit);
    blk->AddStatement(br);
    // 标记出现了break
    return_flag_ = BLOCK::HAS_BREAK;
    // 当前块要连向while_exit
    ControlFlowGraph::ConnectBlocks(blk, while_exit);
    // auto new_blk =  Block::Create(sym_table_, ctx->while_entry,
    // ctx->while_exit); cfgs_.back()->AddBlock(new_blk); //
    // 这个新的block应该是unreachable的
  } else if (auto continue_ = ctx->CONTINUE()) {  // Stmt -> 'continue' ';'
    if (while_list_.empty()) {                  // 不在while循环
      throw std::runtime_error("continue statement not within a loop");
    }
    auto blk = cfgs_.back()->GetLastBlock();
    // auto br = std::make_shared<BRIR>(IRTYPE::IR_I1, blk);
    auto br = BRIR::Create(IRTYPE::IR_I1, blk);
    auto last_while = while_list_.back();
    auto while_entry = std::get<0>(while_list_.back());
    br->AddBranch(while_entry);
    blk->AddStatement(br);
    // 标记存在continue
    return_flag_ = BLOCK::HAS_CONTINUE;
    // 当前所在的block要连向entry
    ControlFlowGraph::ConnectBlocks(blk, while_entry);

    // auto new_blk =  Block::Create(sym_table_, ctx->while_entry, ctx->while_exit);
    // cfgs_.back()->AddBlock(new_blk); // 这个新的block应该是unreachable的
  } else if (auto ret = ctx->RETURN()) {  // Stmt -> 'return' [ Exp ] ';'
    return_flag_ = BLOCK::HAS_RETURN;
    if (ctx->exp() != nullptr) {
      // 如果有返回值，那么visitExp函数需要返回一个vector<IRStatement>，表示计算返回值的IRStatement
      auto exp_ir = std::any_cast<IRPtr>(visit(ctx->exp()));
      // load一下
      LoadVar(exp_ir);
      std::string func_name = sym_table_->GetFuncName();
      auto func_sym = std::dynamic_pointer_cast<FuncSymbol>(sym_table_->FuncLookUp(func_name));
      auto ir_type = toIRType(func_sym->GetReturnType());
      // 考虑类型转换
      if (ir_type == IRTYPE::IR_I32) {
        ZeroExtend(exp_ir);
        Float2Int(exp_ir);
      } else if (ir_type == IRTYPE::IR_FLOAT) {
        Int2Float(exp_ir);
      } else { // void
        throw std::runtime_error("return value in void function");
      }
      // auto ret_stmt = std::make_shared<RetIR>(ir_type);
      // ret_stmt->AddOprnd(exp_ir);
      auto blk = cfgs_.back()->GetLastBlock();
      // blk->AddStatement(exp_ir); // 好像不用add stmt
      // auto br = std::make_shared<BRIR>(IRTYPE::IR_I1, blk);
      auto br = BRIR::Create(IRTYPE::IR_I1, blk);
      br->AddBranch(return_block_);
      blk->AddStatement(br);
      ControlFlowGraph::ConnectBlocks(blk, return_block_);  // 连接
      // 构造phi语句？
      auto return_stmts = return_block_->GetStmts();
      if (return_stmts.size() == 1) {  // 什么都还没放
        // auto phi = std::make_shared<PhiIR>(ir_type);
        auto phi = PhiIR::Create(ir_type);
        phi->AddOprnd(exp_ir);
        phi->AddOprnd(blk->GetStmts()[0]);
        // std::cout << "make br return phi: " << phi->GetID() <<
        // std::endl;
        return_block_->AddStatement(phi);
      } else {
        auto phi = return_stmts[1];
        phi->AddOprnd(exp_ir);
        phi->AddOprnd(blk->GetStmts()[0]);
        // std::cout << "make br return phi: " << phi->GetID() <<
        // std::endl;
      }
    } else {
      // auto ret_stmt = std::make_shared<RetIR>(IRTYPE::IR_VOID);
      auto blk = cfgs_.back()->GetLastBlock();
      // auto br = std::make_shared<BRIR>(IRTYPE::IR_I1, blk);
      auto br = BRIR::Create(IRTYPE::IR_I1, blk);
      br->AddBranch(return_block_);
      blk->AddStatement(br);
      ControlFlowGraph::ConnectBlocks(blk, return_block_);  // 连接
    }
    // auto new_blk =  Block::Create(sym_table_, ctx->while_entry,
    // ctx->while_exit); cfgs_.back()->AddBlock(new_blk); //
    // 这个新的block应该是unreachable的
  } else if (auto block = ctx->block()) {  // Stmt -> Block
    // TODO: 考虑这种情况 "while(cond) { { break; } a = 1; }" 这种情况下，我们的算法将会把a = 1放在br语句之后
    // 我们需要考虑，怎么让外层的block知道这个while已经被break了
    visit(block);
    return nullptr;
  } else if (ctx->exp() != nullptr && ctx->exp()->ident() != nullptr) {  // Stmt -> [ Exp ] ';' function call
    // 此时是函数调用
    visit(ctx->exp());
    // auto blk = cfgs_.back()->GetLastBlock();
    // blk->AddStatement(call_ir);
  } else {  // stmt -> [ exp ] ;
    // Do nothing
  }
  return nullptr;
}

void MyVisitor::SyLibInit() {

  /* putint */
  sym_table_->Insert(
          std::make_shared<FuncSymbol>(
                  Type(BaseType::VOID, SymKind::FUNC, true, true),                    //返回类型
                  "putint",                                                           //函数名
                  std::vector<Type>{Type(BaseType::INT, SymKind::VAR, false, false)}  //函数参数
          )
  );

  /* getint */
  sym_table_->Insert(
          std::make_shared<FuncSymbol>(
                  Type(BaseType::INT, SymKind::FUNC, true, true),
                  "getint",
                  std::vector<Type>{}
          )
  );

  /* putfloat */
  sym_table_->Insert(
          std::make_shared<FuncSymbol>(
                  Type(BaseType::VOID, SymKind::FUNC, true, true),
                  "putfloat",
                  std::vector<Type>{Type(BaseType::FLOAT, SymKind::VAR, false, false)}
          )
  );

  /* getfloat */
  sym_table_->Insert(
          std::make_shared<FuncSymbol>(
                  Type(BaseType::FLOAT, SymKind::FUNC, true, true),
                  "getfloat",
                  std::vector<Type>{}
          )
  );

  /* putch */
  sym_table_->Insert(
          std::make_shared<FuncSymbol>(
                  Type(BaseType::VOID, SymKind::FUNC, true, true),
                  "putch",
                  std::vector<Type>{Type(BaseType::INT, SymKind::VAR, false, false)}
          )
  );

  /* getch */
  sym_table_->Insert(
          std::make_shared<FuncSymbol>(
                  Type(BaseType::INT, SymKind::FUNC, true, true),
                  "getch",
                  std::vector<Type>{}
          )
  );

  /* putarray 打印数组a[0]~a[n-1]的n个数*/
  sym_table_->Insert(
          std::make_shared<FuncSymbol>(
                  Type(BaseType::VOID, SymKind::FUNC, true, true),
                  "putarray",
                  std::vector<Type>{
                          Type(BaseType::INT, SymKind::VAR, false, false),
                          Type(BaseType::INT, SymKind::VAR, false, false, std::vector<size_t>{0})
                  }
          )
  );

  /* getarray 往数组a[0]~a[n-1]读入n个数，并返回n*/
  sym_table_->Insert(
          std::make_shared<FuncSymbol>(
                  Type(BaseType::INT, SymKind::FUNC, true, true),
                  "getarray",
                  std::vector<Type>{Type(BaseType::INT, SymKind::VAR, false, false, std::vector<size_t>{0})}
          )
  );

  /* putfarray */
  sym_table_->Insert(
          std::make_shared<FuncSymbol>(
                  Type(BaseType::VOID, SymKind::FUNC, true, true),
                  "putfarray",
                  std::vector<Type>{
                          Type(BaseType::INT, SymKind::VAR, false, false),
                          Type(BaseType::FLOAT, SymKind::VAR, false, false, std::vector<size_t>{0})
                  }
          )
  );

  /* getfarray */
  sym_table_->Insert(
          std::make_shared<FuncSymbol>(
                  Type(BaseType::INT, SymKind::FUNC, true, true),
                  "getfarray",
                  std::vector<Type>{Type(BaseType::FLOAT, SymKind::VAR, false, false, std::vector<size_t>{0})}
          )
  );

  /* before_main */
  sym_table_->Insert(
          std::make_shared<FuncSymbol>(
                  Type(BaseType::VOID, SymKind::FUNC, true, true),
                  "before_main",
                  std::vector<Type>{}
          )
  );

  /* after_main */
  sym_table_->Insert(
          std::make_shared<FuncSymbol>(
                  Type(BaseType::VOID, SymKind::FUNC, true, true),
                  "after_main",
                  std::vector<Type>{}
          )
  );

  /* _sysy_starttime */
  sym_table_->Insert(
          std::make_shared<FuncSymbol>(
                  Type(BaseType::VOID, SymKind::FUNC, true, true),
                  "_sysy_starttime",
                  std::vector<Type>{Type(BaseType::INT, SymKind::VAR, false, false)}
          )
  );

  /* _sysy_stoptime */
  sym_table_->Insert(
          std::make_shared<FuncSymbol>(
                  Type(BaseType::VOID, SymKind::FUNC, true, true),
                  "_sysy_stoptime",
                  std::vector<Type>{Type(BaseType::INT, SymKind::VAR, false, false)}
          )
  );

  /* putf (类似于printf) */
  sym_table_->Insert(
          std::make_shared<FuncSymbol>(
                  Type(BaseType::VOID, SymKind::FUNC, true, true),
                  "putf",
                  std::vector<Type>{
                          Type(BaseType::STRING, SymKind::VAR, false, false)
                  }
          )
  );
}

auto MyVisitor::GetCFGS() -> std::vector<std::shared_ptr<ControlFlowGraph>> {
  return cfgs_;
}

}  // namespace SCNUCC