//
// Created by 邓实诚 on 2024/5/14.
//
#include "../include/MyVisitor.h"

namespace SCNUCC {
auto MyVisitor::JudgeRetType(const IRPtr &IR) -> IRTYPE::IRTYPE {
  IROP::IROP op = IR->GetOp();
  IRTYPE::IRTYPE type = IR->GetIRType();
  if (op == IROP::IR_CALL && (type == IRTYPE::IR_FLOAT || type == IRTYPE::IR_I32)){
    return type;
  }
  if (type == IRTYPE::IR_CONST_BOOL || type == IRTYPE::IR_CONST_FLOAT || type == IRTYPE::IR_CONST_INT) {
    return type;
  }
  // phi IR的类型
  if (op == IROP::IR_PHI) {
    return type;
  }
  if (op == IROP::IR_ADD || op == IROP::IR_SUB || op == IROP::IR_MUL || op == IROP::IR_SDIV || op == IROP::IR_SREM
      || op == IROP::IR_FTOI || op == IROP::IR_BTOI || (op == IROP::IR_ALLOCA && type == IRTYPE::IR_I32)
      || (op == IROP::IR_PLACEHOLDER && type == IRTYPE::IR_I32) || (op == IROP::IR_XOR && type == IRTYPE::IR_I32)
      || (op == IROP::IR_LRD && type == IRTYPE::IR_I32)) {
    return IRTYPE::IR_I32;
  } else if (op == IROP::IR_ITOF || op == IROP::IR_FADD || op == IROP::IR_FSUB || op == IROP::IR_FMUL ||
             op == IROP::IR_FDIV
             || op == IROP::IR_FREM || op == IROP::IR_ITOF || (op == IROP::IR_ALLOCA && type == IRTYPE::IR_FLOAT)
             || (op == IROP::IR_PLACEHOLDER && type == IRTYPE::IR_FLOAT) || (op == IROP::IR_LRD && type == IRTYPE::IR_FLOAT)) {
    return IRTYPE::IR_FLOAT;
  } else if (op == IROP::IR_IGT || op == IROP::IR_IGE || op == IROP::IR_ILT || op == IROP::IR_IEQ || op == IROP::IR_INE
             || op == IROP::IR_FGT || op == IROP::IR_FGE || op == IROP::IR_FLT || op == IROP::IR_FLT ||
             op == IROP::IR_FLE
             || op == IROP::IR_FEQ || op == IROP::IR_FNE || op == IROP::IR_NEG || op == IROP::IR_AND ||
             op == IROP::IR_NOT
             || op == IROP::IR_NOT || op == IROP::IR_OR || (op == IROP::IR_XOR && type == IRTYPE::IR_I1)) {
    return IRTYPE::IR_I1;
  }
  return IRTYPE::IR_I32;
}

auto MyVisitor::IsConst(IRTYPE::IRTYPE type) -> int {
  return (type == IRTYPE::IR_CONST_FLOAT || type == IRTYPE::IR_CONST_INT || type == IRTYPE::IR_CONST_BOOL);
}

auto MyVisitor::BothConst(IRTYPE::IRTYPE lhs, IRTYPE::IRTYPE rhs) -> int {
  return (IsConst(lhs) && IsConst(rhs));
}

template<typename T>
auto MyVisitor::Calculate(T lVal, T rVal, const std::string &op) -> std::pair<T, IRTYPE::IRTYPE> {
  double newVal;
  IRTYPE::IRTYPE type = IRTYPE::IR_I32;
  if (op == "+") {
    newVal = lVal + rVal;

    // std::cerr << newVal <<std::endl;

  } else if (op == "-") {
    newVal = lVal - rVal;
  } else if (op == "*") {
    newVal = lVal * rVal;
  } else if (op == "/") {
    if (rVal == 0) {
      throw std::runtime_error("error: invalid operands of type 'float' to binary 'operator/'");
    }
    newVal = lVal / rVal;
  } else if (op == "<") {
    newVal = lVal < rVal, type = IRTYPE::IR_CONST_BOOL;
  } else if (op == "<=") {
    newVal = lVal <= rVal, type = IRTYPE::IR_CONST_BOOL;
  } else if (op == ">") {
    newVal = lVal > rVal, type = IRTYPE::IR_CONST_BOOL;
  } else if (op == ">=") {
    newVal = lVal >= rVal, type = IRTYPE::IR_CONST_BOOL;
  } else if (op == "==") {
    newVal = lVal == rVal, type = IRTYPE::IR_CONST_BOOL;
  } else if (op == "!=") {
    newVal = lVal != rVal, type = IRTYPE::IR_CONST_BOOL;
  }
  if (std::is_same<T, float>::value) {
    // 如果参数类型是float，返回pair<float, int>
    if (type == IRTYPE::IR_I32) {
      type = IRTYPE::IR_CONST_FLOAT;
    }
    return std::make_pair(newVal, type);
  } else { /*(std::is_same<T1, int>::value && std::is_same<T2, int>::value)*/
    // 返回pair<int, int>
    if (type == IRTYPE::IR_I32) {
      type = IRTYPE::IR_CONST_INT;
    }
    return std::make_pair(newVal, type);
  }
}

auto MyVisitor::CalcValue(const IRPtr &lhs, const IRPtr &rhs, const std::string &op) -> std::any {
  IRPtr retIR;  // 这个是返回值，计算的结果
  IRTYPE::IRTYPE lType = JudgeRetType(lhs);  // 左边是int还是float还是bool
  IRTYPE::IRTYPE rType = JudgeRetType(rhs);  // 右边是int还是float还是bool
  if (lType == rType && (lType == IRTYPE::IR_CONST_INT || lType == IRTYPE::IR_CONST_BOOL)) {
    int lVal = std::any_cast<int>(lhs->GetConstantVal());
    int rVal = std::any_cast<int>(rhs->GetConstantVal());
    if (op == "%") {
      if (rVal == 0) {
        throw std::runtime_error("Error: invalid operands of type 'float' to binary 'operator%'");
      }
      // retIR = std::make_shared<ConstantIR>(lVal % rVal, IRTYPE::IR_CONST_INT);
      retIR = ConstantIR::Create(lVal % rVal, IRTYPE::IR_CONST_INT);
    } else {
      std::pair<int, IRTYPE::IRTYPE> ret = Calculate<int>(lVal, rVal, op);
      // retIR = std::make_shared<ConstantIR>(ret.first, ret.second);
      retIR = ConstantIR::Create(ret.first, ret.second);
    }

  } else {
    // lhs, rhs其中一个是float，或者都是
    float lVal, rVal;
    if (lType == IRTYPE::IR_CONST_BOOL || lType == IRTYPE::IR_CONST_INT) {
      lVal = (float) (std::any_cast<int>(lhs->GetConstantVal()));
    } else {
      lVal = std::any_cast<float>(lhs->GetConstantVal());
    }
    if (rType == IRTYPE::IR_CONST_BOOL || rType == IRTYPE::IR_CONST_INT) {
      rVal = (float) (std::any_cast<int>(rhs->GetConstantVal()));
    } else {
      rVal = std::any_cast<float>(rhs->GetConstantVal());
    }

    std::pair<float, IRTYPE::IRTYPE> ret = Calculate<float>(lVal, rVal, op);
    // 重新生成一个ConstantIR
    if (ret.second == IRTYPE::IR_CONST_BOOL) {
      // retIR = std::make_shared<ConstantIR>((int) ret.first, ret.second);
      retIR = ConstantIR::Create((int) ret.first, ret.second);
    } else {
      // retIR = std::make_shared<ConstantIR>(ret.first, ret.second);
      retIR = ConstantIR::Create(ret.first, ret.second);
    }
  } // if
  return retIR;
}

auto MyVisitor::GetNextOpt(const IRTYPE::IRTYPE type, const std::string &op) -> IROP::IROP {
  IROP::IROP nextOpt;
  if (op == "+") {
    nextOpt = (type == IRTYPE::IR_I32 ? IROP::IR_ADD : IROP::IR_FADD);
  } else if (op == "-") {
    nextOpt = (type == IRTYPE::IR_I32 ? IROP::IR_SUB : IROP::IR_FSUB);
  } else if (op == "*") {
    nextOpt = (type == IRTYPE::IR_I32 ? IROP::IR_MUL : IROP::IR_FMUL);
  } else if (op == "/") {
    nextOpt = (type == IRTYPE::IR_I32 ? IROP::IR_SDIV : IROP::IR_FDIV);
  } else if (op == "<") {
    nextOpt = (type == IRTYPE::IR_I32 ? IROP::IR_ILT : IROP::IR_FLT);
  } else if (op == ">") {
    nextOpt = (type == IRTYPE::IR_I32 ? IROP::IR_IGT : IROP::IR_FGT);
  } else if (op == "<=") {
    nextOpt = (type == IRTYPE::IR_I32 ? IROP::IR_ILE : IROP::IR_FLE);
  } else if (op == ">=") {
    nextOpt = (type == IRTYPE::IR_I32 ? IROP::IR_IGE : IROP::IR_FGE);
  } else if (op == "==") {
    nextOpt = (type == IRTYPE::IR_I32 ? IROP::IR_IEQ : IROP::IR_FEQ);
  } else if (op == "!=") {
    nextOpt = (type == IRTYPE::IR_I32 ? IROP::IR_INE : IROP::IR_FNE);
  } else if (op == "%") {  // %
    if (type == IRTYPE::IR_FLOAT) {
      throw std::runtime_error("Error: invalid operands of type 'float' to binary 'operator%'");
    } else {
      nextOpt = IROP::IR_SREM;
    }
  }
  return nextOpt;
}

auto MyVisitor::ReadVariable(const std::string &var) -> std::any {
  // 当前作用域和父作用域都查找
  auto sym = sym_table_->VarLookUp(var, SearchScope::ALL_SCOPE);
  if (!sym) {
    throw std::runtime_error("Variable not found");
  }
  auto blk = GetLastBlock();
  IRPtr varIR;
  if (sym->IsConst() && !sym->IsPtr()) { // 是一个常量
    // 返回数值
    if (sym->GetType().base_type_ == BaseType::INT) {
      int val = sym->GetScalerValue<int>();
      // varIR = std::make_shared<ConstantIR>(val, IRTYPE::IR_CONST_INT);
      varIR = ConstantIR::Create(val, IRTYPE::IR_CONST_INT);
    } else {  // 就是float
      float val =sym->GetScalerValue<float>();
      // varIR = std::make_shared<ConstantIR>(val, IRTYPE::IR_CONST_FLOAT);
      varIR = ConstantIR::Create(val, IRTYPE::IR_CONST_FLOAT);
    }
  } else if (sym->IsGlobal() && !sym->IsPtr()) { // 是一个变量
    varIR = gbl_vars[sym->GetString()];
    LoadVar(varIR); // 先尝试lord一下
  } else if (sym->IsGlobal() && sym->IsPtr()) {
    varIR = gbl_vars[sym->GetString()];
  } else { // 其他的
    varIR = blk->ReadVariable(sym);
    LoadVar(varIR); // 先尝试lord一下
  }
  return varIR;
}

auto MyVisitor::ZeroExtend(IRPtr &ir) -> void {
  auto blk = GetLastBlock();
  IRTYPE::IRTYPE type = JudgeRetType(ir);
  int isConst = IsConst(ir->GetIRType());   // 左边是不是立即数
  // 如果最后一条语句返回的是I1且，不是常量
  if (type == IRTYPE::IR_I1 && !isConst) {
    // IRPtr b2I = std::make_shared<BoolToIntIR>();
    IRPtr b2I = BoolToIntIR::Create();
    b2I->AddOprnd(ir);
    blk->AddStatement(b2I);
    ir = b2I;
  } else if (isConst && type == IRTYPE::IR_I1) {
    auto valNum = (std::any_cast<int>(ir->GetConstantVal()));
    // ir = std::make_shared<ConstantIR>(valNum, IRTYPE::IR_CONST_INT);
    ir = ConstantIR::Create(valNum, IRTYPE::IR_CONST_INT);
  }
}

auto MyVisitor::Int2Float(IRPtr &ir) -> void {
  auto blk = GetLastBlock();
  IRTYPE::IRTYPE type = JudgeRetType(ir);
  int isConst = IsConst(ir->GetIRType());   // 左边是不是立即数
  // 如果是常量且不是float，那么直接把数值转为float
  if (isConst && (type == IRTYPE::IR_CONST_BOOL || type == IRTYPE::IR_CONST_INT)) {
    auto valNum = (float) (std::any_cast<int>(ir->GetConstantVal()));
    // ir = std::make_shared<ConstantIR>(valNum, IRTYPE::IR_CONST_FLOAT);
    ir = ConstantIR::Create(valNum, IRTYPE::IR_CONST_FLOAT);
  } else if (!isConst && (type == IRTYPE::IR_I32 || type == IRTYPE::IR_I1)) {
    ZeroExtend(ir);
    // 如果不是常量但返回值不是float，那么加一条转换语句
    // IRPtr int2Float = std::make_shared<IntToFloatIR>();
    IRPtr int2Float = IntToFloatIR::Create();
    int2Float->AddOprnd(ir);
    blk->AddStatement(int2Float);
    ir = int2Float;
  }
}

auto MyVisitor::Float2Int(IRPtr &ir) -> void {
  IRTYPE::IRTYPE type = JudgeRetType(ir);
  auto blk = GetLastBlock();
  if (type == IRTYPE::IR_CONST_FLOAT) {
    // 如果是常量要申请一个新的，不能基于原来的去改变
    // 也不能去释放旧的，因为旧的可能是存在一个variable里面
    int valNum = (int)(std::any_cast<float>(ir->GetConstantVal()));
    // ir = std::make_shared<ConstantIR>(valNum, IRTYPE::IR_CONST_INT);
    ir = ConstantIR::Create(valNum, IRTYPE::IR_CONST_INT);
  } else if (type == IRTYPE::IR_FLOAT) {
    // IRPtr float2Int = std::make_shared<FloatToIntIR>();
    IRPtr float2Int = FloatToIntIR::Create();
    float2Int->AddOprnd(ir);
    blk->AddStatement(float2Int);
    ir = float2Int;
  }
}

auto MyVisitor::Append2Lhs(IRStatementVec &lhs, IRStatementVec &rhs) -> void {
  if (!IsConst(rhs.back()->GetIRType())) {
    for (auto &ir: rhs) {
      lhs.push_back(ir);
    }
  }
}

auto MyVisitor::string2CType(const std::string &type) -> BaseType {
  if (type == "int") {
    return BaseType::INT;
  } else if (type == "float") {
    return BaseType::FLOAT;
  } else if (type == "void") {
    return BaseType::VOID;  
  } else {
    throw std::runtime_error("Unknown type");
  }
}

auto MyVisitor::buildType(BaseType type, SymKind kind, bool is_const, bool is_global,
                          std::vector<size_t> array_shape) -> Type {
  Type t;
  t.kind_ = kind;
  t.base_type_ = type;
  t.is_const_ = is_const;
  t.is_global_ = is_global;
  t.array_shape_ = std::move(array_shape);
  return t;
}

auto MyVisitor::buildFuncSymbol(SysYParser::FuncDefContext *ctx) -> std::shared_ptr<FuncSymbol> {
  Type return_type = buildType(string2CType(ctx->funcType()->getText()), SymKind::FUNC);
  std::string func_name = ctx->ident()->getText();
  if (func_name.size() > 31) {
    func_name = func_name.substr(0,5) + "_";
  }
  auto func_sym = std::make_shared<FuncSymbol>(return_type, func_name);
  return func_sym;
}

auto MyVisitor::string2Number(SysYParser::NumberContext *ctx) -> std::any {
  if (ctx->intConst()) {  // int类型
    std::string valStr = ctx->intConst()->getText();
    int valNum = 0;
    if (valStr[0] == '0') {
      valNum = stoi(valStr, nullptr, 8);
    } else if (valStr.substr(0, 2) == "0x" || valStr.substr(0, 2) == "0X") {
      valNum = stoi(valStr, nullptr, 16);
    } else {
      valNum = stoi(valStr, nullptr, 10);
    }
    return valNum;
  } else {  // float类型
    std::string valStr = ctx->floatConst()->getText();
    float valNum = 0;
    if (valStr.substr(0, 2) == "0x" || valStr.substr(0, 2) == "0X") {
      std::stringstream ss;
      ss << std::hexfloat << valStr;
      ss >> valNum;
    } else {
      valNum = stof(valStr);
    }
    return valNum;
  }
}

auto MyVisitor::GetLastBlock() -> std::shared_ptr<Block> {
  if (sym_table_->IsGlobalScope()) {
    return gbl_init_blk;
  } else {
    return cfgs_.back()->GetLastBlock();
  }
}

auto MyVisitor::LoadVar(IRPtr &ir) -> void {
  if (auto gbl_ir = std::dynamic_pointer_cast<GblVarDeclIR>(ir)) {
    auto blk = GetLastBlock();
    // auto load_ir = std::make_shared<LordIR>(gbl_ir->GetIRType());
    auto load_ir = LoadIR::Create(gbl_ir->GetIRType());
    load_ir->AddOprnd(ir);
    blk->AddStatement(load_ir);
    ir = load_ir;
  } else if (auto alloc_ir = std::dynamic_pointer_cast<AllocaIR>(ir)) {
    auto blk = GetLastBlock();
    // auto load_ir = std::make_shared<LordIR>(alloc_ir->GetIRType());
    auto load_ir = LoadIR::Create(alloc_ir->GetIRType());
    load_ir->AddOprnd(ir);
    blk->AddStatement(load_ir);
    ir = load_ir;
  } else if (auto gep_ir = std::dynamic_pointer_cast<GEPIR>(ir)) {
    auto blk = GetLastBlock();
    // auto load_ir = std::make_shared<LordIR>(gep_ir->GetIRType());
    auto load_ir = LoadIR::Create(gep_ir->GetIRType());
    load_ir->AddOprnd(ir);
    blk->AddStatement(load_ir);
    ir = load_ir;
  }
}

auto MyVisitor::WriteVariable(const std::string &var, const IRPtr &ir) -> void {
  // 当前作用域和父作用域都查找
  auto sym = sym_table_->VarLookUp(var, SearchScope::ALL_SCOPE);
  if (!sym) {
    throw std::runtime_error("Variable not found");
  }
  auto blk = GetLastBlock();

  // 如果这个类型是一个全局变量或者是一个ptr，那么就需要store；否则只需要简单的WriteVariable
  if (sym->IsGlobal()) {
    auto gbl_ir = gbl_vars[sym->GetString()];
    // auto store_ir = std::make_shared<StoreIR>(gbl_ir->GetIRType());
    auto store_ir = StoreIR::Create(gbl_ir->GetIRType());
    // 把值存储到全局变量中
    store_ir->AddOprnd(ir);
    store_ir->AddOprnd(gbl_ir);
    blk->AddStatement(store_ir);
  } else if (auto alloc_ir = std::dynamic_pointer_cast<AllocaIR>(ir)) {
    // Note: sysY产生的LLVM IR不应该有alloca语句，所以这个条件应该永远也不会成立
//    auto store_ir = std::make_shared<StoreIR>(gbl_ir->GetIRType());
//    store_ir->AddOprnd(ir);
//    blk->AddStatement(store_ir);
  } else {
    blk->WriteVariable(sym, ir);
  }
}

auto MyVisitor::backPatch(std::vector<std::shared_ptr<BRIR>> &list, const std::shared_ptr<Block> &block, bool direction) -> void {
  for (auto &ir: list) {
    if(direction){
      ir->AddTrueBranch(block); // 添加出口branch
    } else {
      ir->AddFalseBranch(block);  // 添加出口branch
    }
    // 连接block的控制流
    block->AddParent(ir->GetBlock());
    ir->GetBlock()->AddChild(block);
  }
  // patch完毕，情况整条出口链
  list.clear();
}

auto MyVisitor::genPhiForCond(SysYParser::CondContext *ctx) -> IRPtr {
  std::shared_ptr<SCNUCC::Block>   pre_while_entry = nullptr, pre_while_body = nullptr, pre_while_exit = nullptr;
  if (while_list_.size()) {
    auto last_while = while_list_.back();
    pre_while_entry = std::get<0>(last_while);
    pre_while_body = std::get<1>(last_while);
    pre_while_exit = std::get<2>(last_while);
  }
  // back patch
  auto next_block =  Block::Create(sym_table_, pre_while_entry, pre_while_exit);
  backPatch(true_list.back(), next_block, true);
  backPatch(false_list.back(), next_block, false);
  true_list.pop_back();
  false_list.pop_back();
  // back patch 找到了next_block的所有父block，所以这里只需要AddBlock
  cfgs_.back()->AddBlock(next_block);
  next_block->Seal();

  // 创建phi语句
  // auto const_true = std::make_shared<ConstantIR>(1, IRTYPE::IR_CONST_BOOL);
  // auto const_false = std::make_shared<ConstantIR>(0, IRTYPE::IR_CONST_BOOL);
  auto const_true = ConstantIR::Create(1, IRTYPE::IR_CONST_BOOL);
  auto const_false = ConstantIR::Create(0, IRTYPE::IR_CONST_BOOL);
  // 应该不能直接定死是i1，因为比较的时候是有float比较和int比较还有i1比较的
  // 主要是看接收Phi的那个变量是什么类型
  // auto phi = std::make_shared<PhiIR>(IRTYPE::IR_I1);
  auto phi = PhiIR::Create(IRTYPE::IR_I1);
  next_block->AddStatement(phi);

  // 填充phi语句
  for (const auto &fa_blk: next_block->GetParent()) {
    auto br = std::dynamic_pointer_cast<BRIR>(fa_blk->GetStmts().back());
    auto blk_decl = next_block->GetStmts()[0];
    if (br->GetTrueBranch() == blk_decl && br->GetFalseBranch() == blk_decl) {
      phi->SetIRType(JudgeRetType(br->GetCond()));
      phi->AddOprnd(br->GetCond());
      phi->AddOprnd(fa_blk->GetStmts()[0]);
    } else if (br->GetFalseBranch() == blk_decl) {
      phi->SetIRType(JudgeRetType(const_false));
      phi->AddOprnd(const_false);
      phi->AddOprnd(fa_blk->GetStmts()[0]);
    } else if (br->GetTrueBranch() == blk_decl) {
      phi->SetIRType(JudgeRetType(const_true));
      phi->AddOprnd(const_true);
      phi->AddOprnd(fa_blk->GetStmts()[0]);
    } else {
      throw std::runtime_error("Error: visitCond");
    }
  }

  return phi;
}

auto MyVisitor::GetArrayInitVal(const size_t &cur, std::shared_ptr<VarSymbol> &sym,
                                   SysYParser::ConstInitValContext *ctx) -> std::any {
  // constInitVal -> exp 
  // 此时是递归到了最底层
  if (ctx->constExp() != nullptr) {
    auto exp_ir = std::any_cast<IRPtr>(visit(ctx));
    LoadVar(exp_ir);
    if (sym->GetType().base_type_ == BaseType::INT) {
      Float2Int(exp_ir);  // 肯定是constantIR
    } else {
      Int2Float(exp_ir);  // 肯定是constantIR
    }
    return exp_ir;
  }
  // constInitVal -> LBRACE (constInitVal (COMMA constInitVal)*)? RBRACE;
  // 这个val这里是很万金油的
  // 如果下一层还是一个数组，会返回一个vector
  // 如果到了最后一层，那么就会返回一个IR
  // 使用的时候需要根据数组的维度逐步cast
  std::vector<std::any> val;
  size_t num = ctx->constInitVal().size();
  size_t i = 0;
  while(i < num) {
    if (ctx->constInitVal(i)->constExp() == nullptr) {
      // 这个还不是最底层，下一层返回的是一个vector
      auto vt = GetArrayInitVal(cur + 1, sym, ctx->constInitVal(i++));
      val.push_back(vt);
    } else {
      // 这时候到了最底层
      if (cur == sym->Dim() - 1) {  
        // 这个时候其实是正确地到了最后一层
        auto vt = GetArrayInitVal(cur + 1, sym, ctx->constInitVal(i++));
        val.push_back(vt);
      } else {
        // 这个时候是需要自己去构造位置的 接下来的所有constInitVal, 都是指向一个constExp的
        // int c[4][2] = {{1,2}, {3, 4}, 5, 6, 7, 8}
        // 访问到了5 6 7 8
        auto ctx_vec = ctx->constInitVal();
        auto vt = GetArray(cur + 1, sym, ctx_vec, i);
        val.push_back(vt);
      }
    }
  }
  return val;
}

/**
 * GetArray调用时，constInivalCtx都是直接推导出一个constExp
 * 此时GetArray把每个一个constInitValCtx返回的exp组装成为一个数组，并且返回
 * */
auto MyVisitor::GetArray(const size_t &cur, std::shared_ptr<VarSymbol> &sym, 
                      std::vector<SysYParser::ConstInitValContext*>& ctxs, size_t& idx) -> std::vector<std::any> {
  std::vector<std::any> val;
  size_t val_num = sym->GetType().array_shape_[cur];
  if (cur != sym->Dim() - 1) { // 没有到达最底层
    for (size_t i = 0; i < val_num; i++) {
      if (idx >= ctxs.size()) {
        break;
      }
      if (ctxs[idx]->constExp() != nullptr) {
        val.push_back(GetArray(cur + 1, sym, ctxs, idx));
      } else {
        val.push_back(GetArrayInitVal(cur + 1, sym, ctxs[idx++]));
      }
    }
  } else {
    for (size_t i = 0; i < val_num; i++) {
      if (idx >= ctxs.size()) {
        break;
      }
      if (ctxs[idx]->constExp() != nullptr) {
        if (cur == sym->Dim() - 1) {
          auto exp_ir = std::any_cast<IRPtr>(visit(ctxs[idx++]));
          if (sym->GetType().base_type_ == BaseType::INT) {
            Float2Int(exp_ir);
          } else {
            ZeroExtend(exp_ir);
            Int2Float(exp_ir);
          }
          val.push_back(exp_ir);
        }
      }
    }
  }
  return val;
}


auto MyVisitor::GetArrayInitVal(const size_t &cur, std::shared_ptr<VarSymbol> &sym, 
                                  SysYParser::InitValContext *ctx) -> std::any {
  // constInitVal -> exp 
  // 此时是递归到了最底层
  if (ctx->exp() != nullptr) {
    auto exp_ir = std::any_cast<IRPtr>(visit(ctx));
    LoadVar(exp_ir);
    if (sym->GetType().base_type_ == BaseType::INT) {
      Float2Int(exp_ir);  // 肯定是constantIR
    } else {
      Int2Float(exp_ir);  // 肯定是constantIR
    }
    return exp_ir;
  }
  // constInitVal -> LBRACE (constInitVal (COMMA constInitVal)*)? RBRACE;
  // 这个val这里是很万金油的
  // 如果下一层还是一个数组，会返回一个vector
  // 如果到了最后一层，那么就会返回一个IR
  // 使用的时候需要根据数组的维度逐步cast
  std::vector<std::any> val;
  size_t num = ctx->initVal().size();
  size_t i = 0;
  while(i < num) {
    if (ctx->initVal(i)->exp() == nullptr) {
      // 这个还不是最底层，下一层返回的是一个vector
      auto vt = GetArrayInitVal(cur + 1, sym, ctx->initVal(i++));
      val.push_back(vt);
    } else {
      // 这时候到了最底层
      if (cur == sym->Dim() - 1) {  
        // 这个时候其实是正确地到了最后一层
        auto vt = GetArrayInitVal(cur + 1, sym, ctx->initVal(i++));
        val.push_back(vt);
      } else {
        // 这个时候是需要自己去构造位置的 接下来的所有constInitVal, 都是指向一个constExp的
        // int c[4][2] = {{1,2}, {3, 4}, 5, 6, 7, 8}
        // 访问到了5 6 7 8
        auto ctx_vec = ctx->initVal();
        auto vt = GetArray(cur + 1, sym, ctx_vec, i);
        val.push_back(vt);
      }
    }
  }
  return val;
}

auto MyVisitor::GetArray(const size_t &cur, std::shared_ptr<VarSymbol> &sym, 
                      std::vector<SysYParser::InitValContext*>& ctxs, size_t& idx) -> std::vector<std::any> {
  std::vector<std::any> val;
  size_t val_num = sym->GetType().array_shape_[cur];
  if (cur != sym->Dim() - 1) { // 没有到达最底层
    for (size_t i = 0; i < val_num; i++) {
      if (idx >= ctxs.size()) {
        break;
      }
      if (ctxs[idx]->exp() != nullptr) {
        val.push_back(GetArray(cur + 1, sym, ctxs, idx));
      } else {
        val.push_back(GetArrayInitVal(cur + 1, sym, ctxs[idx++]));
      }
    }
  } else {
    for (size_t i = 0; i < val_num; i++) {
      if (idx >= ctxs.size()) {
        break;
      }
      if (ctxs[idx]->exp() != nullptr) {
        if (cur == sym->Dim() - 1) {
          auto exp_ir = std::any_cast<IRPtr>(visit(ctxs[idx++]));
          if (sym->GetType().base_type_ == BaseType::INT) {
            Float2Int(exp_ir);
          } else {
            ZeroExtend(exp_ir);
            Int2Float(exp_ir);
          }
          val.push_back(exp_ir);
        }
      }
    }
  }
  return val;
}

auto MyVisitor::LocalInitArray(const std::shared_ptr<VarSymbol> &symbol, IRPtr &oprnd,
                               IRStatementVec &indices, const std::any &values) -> void {
  BaseType type = symbol->GetType().base_type_;
  auto blk = cfgs_.back()->GetLastBlock();
  if (values.type() == typeid(std::vector<std::any>)) {
    // 还套着一层
    auto vt = std::any_cast<std::vector<std::any>>(values);
    for (size_t i = 0; i < vt.size(); i++) {
      // indices.emplace_back(std::make_shared<ConstantIR>((int)i, IRTYPE::IR_CONST_INT));
      indices.emplace_back(ConstantIR::Create((int)i, IRTYPE::IR_CONST_INT));
      // 有了这个i，就可以先生成这一层的GEP
      // 这里用I32和FLOAT不知道会不会出问题
      // IRPtr gep = std::make_shared<GEPIR>(type == BaseType::INT ? IRTYPE::IR_I32 : IRTYPE::IR_FLOAT, symbol, indices);
      IRPtr gep = GEPIR::Create(type == BaseType::INT ? IRTYPE::IR_I32 : IRTYPE::IR_FLOAT, symbol, indices);
      if (indices.size() == 1) {
        // 如果才是第一层, 那么oprnd其实是ArrayDeclIR
        auto oprand = std::dynamic_pointer_cast<ArrayDeclIR>(oprnd);
        gep->AddOprnd(oprand);
      } else {
        // 否则是gepIR
        auto oprand = std::dynamic_pointer_cast<GEPIR>(oprnd); 
        gep->AddOprnd(oprand);
      }
      gep->AddOprnd(indices.back());
      blk->AddStatement(gep);
      LocalInitArray(symbol, gep, indices, vt[i]);
      indices.pop_back();
    }
  } else {
    // 生成一连串的GEP，然后store
    // store语句
    // auto store = std::make_shared<StoreIR>(type == BaseType::INT ? IRTYPE::IR_I32 : IRTYPE::IR_FLOAT);
    auto store = StoreIR::Create(type == BaseType::INT ? IRTYPE::IR_I32 : IRTYPE::IR_FLOAT);
    IRPtr stmt = std::any_cast<IRPtr>(values);
    store->AddOprnd(stmt);
    store->AddOprnd(oprnd);
    blk->AddStatement(store);
  }
}

auto MyVisitor::GlobalInitArray(const std::shared_ptr<VarSymbol> &symbol, 
                                const std::any &values, const int cur,
                                std::stringstream &ssa) -> void {
  Type type = symbol->GetType();
  int dimension = type.array_shape_.size();
  if (cur == dimension) {  // 到达最底层
    auto ir = std::any_cast<IRPtr>(values);
    if (type.base_type_ == BaseType::INT) {
      ssa << "i32 " << std::any_cast<int>(ir->GetConstantVal());
    } else {
      double val = (double)std::any_cast<float>(ir->GetConstantVal());
      ssa << "float " << Util::GetIEEE754(val);
    }
  } else {
    
    
    auto vt = std::any_cast<std::vector<std::any>>(values);
    std::vector<size_t> vt_ = type.array_shape_;
    size_t num = vt_[cur];

    for (size_t i = cur; i < vt_.size(); i++) {
      ssa << "[" << vt_[i] << " x";
    }
    if (type.base_type_ == BaseType::INT) ssa << " i32";
    else ssa << " float";
    for (size_t i = cur; i < vt_.size(); i++) ssa << "]";
    
    ssa << " [";

    for (size_t i = 0; i < num; i++) {
      if (i) {
        ssa << ", ";
      } // if
      if (i < vt.size()) {

        GlobalInitArray(symbol, vt[i], cur + 1, ssa);
      } else {
        if (cur + 1 < dimension) { // 此时有一整行没有初始化
          for (size_t i = cur+1; i < vt_.size(); i++) {
            ssa << "[" << vt_[i] << " x";
          }
          if (type.base_type_ == BaseType::INT) ssa << " i32";
          else ssa << " float";
          for (size_t i = cur+1; i < vt_.size(); i++) ssa << "]";
          
          ssa << " zeroinitializer";
        } else {  // 此时是有一个值没有初始化
          if (type.base_type_ == BaseType::INT) {
            ssa << "i32 0";
          } else {
            ssa << "float " << Util::GetIEEE754(0.0); // IEEE 754编码
          } // if
        } // if
      }  // if
    } //for
    ssa << "]";
  }
}

}