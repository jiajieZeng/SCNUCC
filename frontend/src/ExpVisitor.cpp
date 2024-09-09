//
// Created by 邓实诚 on 2024/5/15.
//
#include "../include/MyVisitor.h"

#include <fstream>
#include <sstream>

/**
 * 如果是对于ConstantIR的计算
 * 是会把值先计算出来的
 * int a = 10;  ConstantIR
 * int b = 20 + a; 1ConstantIR 30
 * 
 * 
*/

namespace SCNUCC {

std::any MyVisitor::visitExp(SysYParser::ExpContext *ctx) {
  if (ctx->lVal()) {
    // exp -> lVal
    auto ir = std::any_cast<IRPtr>(visit(ctx->lVal()));
    return ir;
  } else if (ctx->ident() != nullptr) {
    // exp -> ident LPAREN (funcRParams)? RPAREN
    auto func_name = ctx->ident()->getText();
    if (func_name.size() > 31) {
      func_name = func_name.substr(0,5) + "_";
    }

    // runtime库头文件有计时函数的宏定义，需要改下名字

    if (func_name == "stoptime") func_name = "_sysy_stoptime";
    else if (func_name == "starttime") func_name = "_sysy_starttime";

    auto func_symbol = sym_table_->FuncLookUp(func_name);

    // 特判计时函数
    if (func_name == "_sysy_stoptime" || func_name == "_sysy_starttime") {
      int lineno = ctx->getStart()->getLine();
      // auto call_ir = std::make_shared<CallIR>(IRTYPE::IR_VOID, func_name, func_symbol->GetAllParams());
      auto call_ir = CallIR::Create(IRTYPE::IR_VOID, func_name, func_symbol->GetAllParams());
      // IRPtr numIR = std::make_shared<ConstantIR>(lineno, IRTYPE::IR_CONST_INT);
      IRPtr numIR = ConstantIR::Create(lineno, IRTYPE::IR_CONST_INT);
      auto exp_ir = std::any_cast<IRPtr>(numIR);
      ZeroExtend(exp_ir);
      call_ir->AddOprnd(exp_ir);
      GetLastBlock()->AddStatement(call_ir);
      return std::static_pointer_cast<IRStatement>(call_ir);
    }
    if (func_symbol == nullptr) {
      std::string msg = "Error: Called unkwon function " + func_name;
      throw std::runtime_error(msg);
    }
    IRTYPE::IRTYPE ret_type = IRTYPE::IR_VOID;
    auto rtype = func_symbol->GetReturnType();
    if (rtype == BaseType::INT) {
      ret_type = IRTYPE::IR_I32;
    } else if (rtype == BaseType::FLOAT) {
      ret_type = IRTYPE::IR_FLOAT;
    }

    // 记录下函数各个参数的类型


    // putf的第二个参数是可变参数，扫描后才能确定，因此在这里添加
    if (func_name == "putf") {
      //重置putf的funcsymbol
      func_symbol->ResetParams();
      func_symbol->PushPackParam(Type(BaseType::STRING, SymKind::VAR, false, false));
      // %d %c %f 
      std::string rpstring = ctx->funcRParams()->string()->getText();
      rpstring = rpstring.substr(1, rpstring.size() - 2); //去掉双引号
      Util::PushBackgstring(rpstring); //将第一个实参(即字符串)加进全局字符串中

      for (int i = 0; i < rpstring.size() - 1; i++) {
        if (rpstring[i] == '%') {
          char t = rpstring[i + 1];

          if (t == 'd') { func_symbol->PushPackParam(Type(BaseType::INT, SymKind::VAR, false, false)); }
          else if (t == 'c') { func_symbol->PushPackParam(Type(BaseType::INT, SymKind::VAR, false, false)); }
          else if (t == 'f') { func_symbol->PushPackParam(Type(BaseType::FLOAT, SymKind::VAR, false, false)); }
          else {
            std::string msg = "Error: unsupported type for putf";
            throw std::runtime_error(msg);
          }
        }
      }
    }



    std::vector<Type> all_params = func_symbol->GetAllParams();
    // auto call_ir = std::make_shared<CallIR>(ret_type, func_name, all_params);
    auto call_ir = CallIR::Create(ret_type, func_name, all_params);
    if (ctx->funcRParams() != nullptr) {
      // 遍历传入的参数 形参

      for (size_t i = 0; i < ctx->funcRParams()->exp().size(); i++) {
        auto exp_ir = std::any_cast<IRPtr>(visit(ctx->funcRParams()->exp(i)));
        // 这里要判断拿到的是不是数组名
        if (all_params[i].array_shape_.size()) {  // 是数组
          call_ir->AddOprnd(exp_ir);  // 这里其实是gep IR
        } else {
          LoadVar(exp_ir);  // 如果是GEP，需要load
          ZeroExtend(exp_ir);
          // 这里先只考虑类型转换
          if (all_params[i].base_type_ == BaseType::INT) {
            Float2Int(exp_ir);
          } else if (all_params[i].base_type_ == BaseType::FLOAT) {
            Int2Float(exp_ir);
          }
          call_ir->AddOprnd(exp_ir);
        }
      }
    }

    auto blk = GetLastBlock();
    blk->AddStatement(call_ir);
    return std::static_pointer_cast<IRStatement>(call_ir);
  } else if (ctx->number()) {
    // exp -> number

    auto blk = GetLastBlock();
    IRPtr numIR;
    if (ctx->number()->intConst()) {    // int类型
      auto intVal = std::any_cast<int>((visitNumber(ctx->number())));
      // numIR = std::make_shared<ConstantIR>(intVal, IRTYPE::IR_CONST_INT);
      numIR = ConstantIR::Create(intVal, IRTYPE::IR_CONST_INT);
    } else {  // float类型
      auto floatVal = std::any_cast<float>((visitNumber(ctx->number())));
      // numIR = std::make_shared<ConstantIR>(floatVal, IRTYPE::IR_CONST_FLOAT);
      numIR = ConstantIR::Create(floatVal, IRTYPE::IR_CONST_FLOAT);
    }
    blk->AddStatement(numIR);
    return numIR;
  } else if (ctx->unaryOp()) {
    // exp -> unaryOp exp
    // 一元运算
    return MyVisitor::visitUnaryExp(ctx);
  } else if (ctx->op != nullptr) {
    // 加减乘除模
    return MyVisitor::visitMulAddExp(ctx);
  } else if (ctx->LPAREN()) {
    // exp -> LPAREN exp RPAREN
    return MyVisitor::visitExp(ctx->exp(0));
  }
  return nullptr;
}

std::any MyVisitor::visitLVal(SysYParser::LValContext *ctx) {
  std::string var_name = ctx->ident()->getText();
  if (var_name.size() > 31) {
    var_name = var_name.substr(0,5) + "_";
  }
  // 在当前作用域查找
  std::shared_ptr<Scope> cur_scope = sym_table_->GetCurrentScope();
  // 这个肯定是VarSymbol 
  std::shared_ptr<VarSymbol> var_sym = std::dynamic_pointer_cast<VarSymbol>(
          cur_scope->VarLookUp(var_name, SearchScope::CUR_SCOPE));
  // 如果这个作用域没找到，就可以去到父作用域找
  if (var_sym == nullptr) {
    var_sym = std::dynamic_pointer_cast<VarSymbol>(cur_scope->VarLookUp(var_name, SearchScope::ALL_SCOPE));
  }
  Type var_type = var_sym->GetType();
  if (var_sym->IsPtr()) {
    // 是数组
    auto blk = cfgs_.back()->GetLastBlock();
    auto var_ir = std::any_cast<IRPtr>(MyVisitor::ReadVariable(var_name));
    IRStatementVec indices;
    BaseType type = var_sym->GetType().base_type_;
    IRPtr gep = nullptr;
    for (size_t i = 0; i < ctx->exp().size(); i++) {  // 拿出索引
      auto exp_ir = std::any_cast<IRPtr>(visit(ctx->exp(i)));
      LoadVar(exp_ir);
      ZeroExtend(exp_ir);
      Float2Int(exp_ir);
      indices.push_back(exp_ir);
      // IRPtr new_gep = std::make_shared<GEPIR>(type == BaseType::INT ? IRTYPE::IR_I32 : IRTYPE::IR_FLOAT, var_sym,
      //                                         indices);
      IRPtr new_gep = GEPIR::Create(type == BaseType::INT ? IRTYPE::IR_I32 : IRTYPE::IR_FLOAT, var_sym,
                                              indices);
      if (i == 0) {
        new_gep->AddOprnd(var_ir);  // 加入数组的声明IR
      } else {
        new_gep->AddOprnd(gep);  // 加入上一条gep
      }
      gep = new_gep;
      new_gep->AddOprnd(indices.back());
      blk->AddStatement(gep);
    }
    if (gep == nullptr) { // 此时只给了数组名
      // indices.push_back(std::make_shared<ConstantIR>(0, IRTYPE::IR_CONST_INT));
      indices.push_back(ConstantIR::Create(0, IRTYPE::IR_CONST_INT));
      // gep = std::make_shared<GEPIR>(type == BaseType::INT ? IRTYPE::IR_I32 : IRTYPE::IR_FLOAT, var_sym, indices);
      gep = GEPIR::Create(type == BaseType::INT ? IRTYPE::IR_I32 : IRTYPE::IR_FLOAT, var_sym, indices);
      blk->AddStatement(gep);
      gep->AddOprnd(var_ir);
      gep->AddOprnd(indices.back());
    }
    return gep;
  } else {
    auto var_ir = std::any_cast<IRPtr>(MyVisitor::ReadVariable(var_name));
    return var_ir;
  }
  return nullptr;
}


std::any MyVisitor::visitUnaryExp(SysYParser::ExpContext *ctx) {
  auto IR = std::any_cast<IRPtr>(visit(ctx->exp(0)));  // 这个IR在visitExp中生成并且已经放入到blk中了
  std::shared_ptr<Block> blk;
  if (sym_table_->IsGlobalScope()) {
    blk = gbl_init_blk;
  } else {
    blk = cfgs_.back()->GetLastBlock();
  }
  // 如果是数组元素进行加减乘除，load一下
  LoadVar(IR);
  if (ctx->unaryOp()->ADD()) {
    // 正号，不需要做任何事情
    // +a +b
    // load一下
    return IR;

  } else if (ctx->unaryOp()->SUB()) {
    // 如果是负数
    // a
    // a = 0 - a
    IRTYPE::IRTYPE type = JudgeRetType(IR); // 获取这条ir的返回值类型
    // 如果是INT或者FLOAT，那么vector里面肯定只有一个ir
    if (type == IRTYPE::IR_CONST_INT) {
      int valNum = std::any_cast<int>(IR->GetConstantVal());
      valNum *= -1;
      // IRPtr newIR = std::make_shared<ConstantIR>(valNum, IRTYPE::IR_CONST_INT);
      IRPtr newIR = ConstantIR::Create(valNum, IRTYPE::IR_CONST_INT);
      blk->AddStatement(newIR);
      return newIR;
    } else if (type == IRTYPE::IR_CONST_FLOAT) {
      auto valNum = std::any_cast<float>(IR->GetConstantVal());
      valNum *= -1.0;
      // IRPtr newIR = std::make_shared<ConstantIR>(valNum, IRTYPE::IR_CONST_FLOAT);
      IRPtr newIR = ConstantIR::Create(valNum, IRTYPE::IR_CONST_FLOAT);
      blk->AddStatement(newIR);
      return newIR;
    } else {
      IRTYPE::IRTYPE nextType = JudgeRetType(IR);
      // 0的IR
      // IRPtr zeroIR = std::make_shared<ConstantIR>(0, nextType == IRTYPE::IR_FLOAT ? IRTYPE::IR_CONST_FLOAT
      //                                                                             : IRTYPE::IR_CONST_INT);
      IRPtr zeroIR = ConstantIR::Create(0, nextType == IRTYPE::IR_FLOAT ? IRTYPE::IR_CONST_FLOAT
                                                                            : IRTYPE::IR_CONST_INT);
      // 算术运算
      // IRPtr ariIR = std::make_shared<ArithmeticIR>(nextType == IRTYPE::IR_FLOAT ? IROP::IR_FSUB
                                                                                // : IROP::IR_SUB, nextType);   // 减法
      IRPtr ariIR = ArithmeticIR::Create(nextType == IRTYPE::IR_FLOAT ? IROP::IR_FSUB
                                                                                : IROP::IR_SUB, nextType);
      // 操作数
      ariIR->AddOprnd(zeroIR);
      ariIR->AddOprnd(IR);
      // 0 - exp
      blk->AddStatement(zeroIR);
      blk->AddStatement(ariIR);
      return ariIR;
    } // if
  } else {  // 此时是 NOT 函数
    IRTYPE::IRTYPE type = JudgeRetType(IR); // 根据LLVMIR的返回值进行判断
    if (type == IRTYPE::IR_CONST_INT) {
      int valNum = std::any_cast<int>(IR->GetConstantVal());
      valNum = !valNum;
      // IRPtr newIR = std::make_shared<ConstantIR>(valNum, IRTYPE::IR_CONST_INT);
      IRPtr newIR = ConstantIR::Create(valNum, IRTYPE::IR_CONST_INT);
      blk->AddStatement(newIR);
      return newIR;
    } else if (type == IRTYPE::IR_CONST_FLOAT) {
      auto valNum = std::any_cast<float>(IR->GetConstantVal());
      int newValNum = valNum == 0.0; // !
      // IRPtr newIR = std::make_shared<ConstantIR>(newValNum, IRTYPE::IR_CONST_INT);
      IRPtr newIR = ConstantIR::Create(newValNum, IRTYPE::IR_CONST_INT);
      blk->AddStatement(newIR);
      return newIR;
    } else {
      // 不是常数了
      // 把IR拿出来
      // 和0进行比较
      // xor
      ZeroExtend(IR);
      // IRPtr cmpIR = std::make_shared<CmpIR>(type == IRTYPE::IR_I32 ? IROP::IR_INE : IROP::IR_FNE, type);
      IRPtr cmpIR = CmpIR::Create(type == IRTYPE::IR_I32 ? IROP::IR_INE : IROP::IR_FNE, type);
      cmpIR->AddOprnd(IR);
      // IRPtr zeroIR = std::make_shared<ConstantIR>(0, type == IRTYPE::IR_I32 ? IRTYPE::IR_CONST_INT
      //                                                                       : IRTYPE::IR_CONST_FLOAT); // 0
      IRPtr zeroIR = ConstantIR::Create(0, type == IRTYPE::IR_I32 ? IRTYPE::IR_CONST_INT
                                                                            : IRTYPE::IR_CONST_FLOAT); // 0
      cmpIR->AddOprnd(zeroIR);
      // IRPtr xorIR = std::make_shared<LogicIR>(IRTYPE::IR_I1);
      IRPtr xorIR = LogicIR::Create(IRTYPE::IR_I1);
      xorIR->AddOprnd(cmpIR);

      blk->AddStatement(cmpIR);
      blk->AddStatement(zeroIR);
      blk->AddStatement(xorIR);

      return xorIR;
    } /* else if (type == IRTYPE::IR_I1) {
      IRPtr xorIR = std::make_shared<LogicIR>();
      xorIR->AddOprnd(ir);
      IRs.push_back(xorIR);
    }*/ // if

  } // if
  return nullptr;
}

std::any MyVisitor::visitNumber(SysYParser::NumberContext *ctx) {
  // 返回数字
  if (ctx->intConst()) {  // int类型
    std::string valStr = ctx->intConst()->getText();
    int valNum = 0;
    if (valStr.substr(0, 2) == "0x" || valStr.substr(0, 2) == "0X") {
      valNum = stoi(valStr, nullptr, 16);
    } else if (valStr[0] == '0') {
      valNum = stoi(valStr, nullptr, 8);
    } else {
      valNum = stoi(valStr, nullptr, 10);
    }
    return valNum;
  } else {  // float类型
    std::string valStr = ctx->floatConst()->getText();
    float valNum = 0;
    valNum = stof(valStr);
    return valNum;
  }
}

/**
 * 加减乘除
*/
std::any MyVisitor::visitMulAddExp(SysYParser::ExpContext *ctx) {
  auto lhs = std::any_cast<IRPtr>(visit(ctx->exp(0)));
  auto rhs = std::any_cast<IRPtr>(visit(ctx->exp(1)));
  IRPtr newIR;  // 新生成的IRStatement
  auto blk = GetLastBlock();
  // 如果是数组元素进行加减乘除，load一下
  LoadVar(lhs);
  LoadVar(rhs);
  // 先来一个0扩展
  // 尽早放，如果本来就是i32
  ZeroExtend(lhs);
  ZeroExtend(rhs);
  IRTYPE::IRTYPE lType = JudgeRetType(lhs);  // 左边是int还是float
  IRTYPE::IRTYPE rType = JudgeRetType(rhs);  // 右边是int还是float
  // 计算
  // 两边都是常量
  if (BothConst(lhs->GetIRType(), rhs->GetIRType())) {
    // 编译时就计算出来了
    newIR = std::any_cast<IRPtr>(CalcValue(lhs, rhs, ctx->op->getText()));
    blk->AddStatement(newIR);
    return newIR;
  } else {
    // 大家都是int类型的 
    if ((lType == IRTYPE::IR_I32 || lType == IRTYPE::IR_CONST_INT) &&
        (rType == IRTYPE::IR_I32 || rType == IRTYPE::IR_CONST_INT)) {
      // newIR = std::make_shared<ArithmeticIR>(GetNextOpt(IRTYPE::IR_I32, ctx->op->getText()), IRTYPE::IR_I32);
      newIR = ArithmeticIR::Create(GetNextOpt(IRTYPE::IR_I32, ctx->op->getText()), IRTYPE::IR_I32);
    } else {
      // 其中一个是float，或者都是
      // 转成FLoat
      // 先用了再说，float不改变
      Int2Float(lhs);
      Int2Float(rhs);
      // newIR = std::make_shared<ArithmeticIR>(GetNextOpt(IRTYPE::IR_FLOAT, ctx->op->getText()), IRTYPE::IR_FLOAT);
      newIR = ArithmeticIR::Create(GetNextOpt(IRTYPE::IR_FLOAT, ctx->op->getText()), IRTYPE::IR_FLOAT);
    } // if
    newIR->AddOprnd(lhs);
    newIR->AddOprnd(rhs);
    blk->AddStatement(newIR);
    return newIR;
  } // if
}


std::any MyVisitor::visitCond(SysYParser::CondContext *ctx) {
  // if (a || b) 里面的 || 
  auto logic_parent = dynamic_cast<SysYParser::CondContext *>(ctx->parent);
  // if (a)
  auto stmt_parent = dynamic_cast<SysYParser::StmtContext *>(ctx->parent);
  if (ctx->exp() != nullptr) {  // cond -> exp
    auto blk = GetLastBlock();  // LastBlcok 当前的block
    auto ir = std::any_cast<IRPtr>(visit(ctx->exp()));
    // 如果是数组元素进行加减乘除，load一下
    LoadVar(ir);
    // 先0扩展
    ZeroExtend(ir);

    if ((logic_parent && (logic_parent->op->getText() == "&&" || logic_parent->op->getText() == "||"))
        || (stmt_parent && (stmt_parent->IF() != nullptr || stmt_parent->WHILE() != nullptr))) {
      // 如果是逻辑运算符，则需要生成并返回一个比较语句
      // 或者是if (exp)   while (exp)
      auto ir_type = JudgeRetType(ir);
      IRTYPE::IRTYPE next_type;
      IROP::IROP next_op;
      if (ir_type == IRTYPE::IR_CONST_INT || ir_type == IRTYPE::IR_I32) {
        next_op = IROP::IR_INE;
        next_type = IRTYPE::IR_I32;
      } else {  // 一定是IR_FLOAT || IR_CONST_FLOAT
        next_op = IROP::IR_FNE;
        next_type = IRTYPE::IR_FLOAT;
      }
      // IRPtr zeroIR = std::make_shared<ConstantIR>(0, next_type == IRTYPE::IR_I32 ? IRTYPE::IR_CONST_INT
      //                                                                            : IRTYPE::IR_CONST_FLOAT);
      IRPtr zeroIR = ConstantIR::Create(0, next_type == IRTYPE::IR_I32 ? IRTYPE::IR_CONST_INT
                                                                                 : IRTYPE::IR_CONST_FLOAT);
      // auto cmpIR = std::make_shared<CmpIR>(next_op, next_type);
      auto cmpIR = CmpIR::Create(next_op, next_type);
      cmpIR->AddOprnd(ir);
      cmpIR->AddOprnd(zeroIR);

      blk->AddStatement(zeroIR);
      blk->AddStatement(cmpIR);

      // 新建一个BRIR
      // auto br = std::make_shared<BRIR>(IRTYPE::IR_I1, blk);
      auto br = BRIR::Create(IRTYPE::IR_I1, blk);
      br->AddOprnd(cmpIR);
      true_list.emplace_back();
      true_list.back().push_back(br);
      false_list.emplace_back();
      false_list.back().push_back(br);
      blk->AddStatement(br);

      return cmpIR;
    } else {
      return ir;
    }
  } else if (ctx->op->getText() == "&&" || ctx->op->getText() == "||") {  // 逻辑运算
    auto cfg = cfgs_.back();
    auto left_block = cfg->GetLastBlock();
    // 构造第一个cond block
    visit(ctx->cond(0));
    std::shared_ptr<SCNUCC::Block> pre_while_entry = nullptr, pre_while_body = nullptr, pre_while_exit = nullptr;
    // back patch
    if (!while_list_.empty()) {
      auto last_while = while_list_.back();
      pre_while_entry = std::get<0>(last_while);
      pre_while_body = std::get<1>(last_while);
      pre_while_exit = std::get<2>(last_while);
    }
    auto right_block =  Block::Create(sym_table_, pre_while_entry, pre_while_exit);
    if (ctx->op->getText() == "&&") {
      backPatch(true_list.back(), right_block, true);
      true_list.pop_back();
      // false_list没有办法处理就合并到父节点的false_list中，让父节点去处理
      // 避免出现这种问题的问题是https://gitlab.eduxiji.net/T202410574202994/scnucc/-/issues/30
      if (false_list.size() >= 2) {
        auto& parent_list = false_list[false_list.size() - 2];
        parent_list.insert(parent_list.end(), false_list.back().begin(), false_list.back().end());
        false_list.pop_back();
      }
    } else {
      backPatch(false_list.back(), right_block, false);
      false_list.pop_back();
      if (true_list.size() >= 2) {
        auto& parent_list = true_list[true_list.size() - 2];
        parent_list.insert(parent_list.end(), true_list.back().begin(), true_list.back().end());
        true_list.pop_back();
      }
    }
    // 在back patch中已经完成了连接，所以只需要AddBlock
    cfg->AddBlock(right_block);
    right_block->Seal();

    // 构造第二个cond block，产生的IR将会放在right_block中
    visit(ctx->cond(1));

    if (false_list.size() >= 2) {
      auto& parent_list = false_list[false_list.size() - 2];
      false_list[false_list.size() - 2].insert(parent_list.end(), false_list.back().begin(), false_list.back().end());
      false_list.pop_back();
    }

    if (true_list.size() >= 2) {
      auto& parent_list = true_list[true_list.size() - 2];
      parent_list.insert(parent_list.end(), true_list.back().begin(), true_list.back().end());
      true_list.pop_back();
    }

    // 如果父节点是比较运算符，那么需要返回一个phi语句
    // 处理的是" (con1) == (cond2) " 这种情况
    // auto parent = dynamic_cast<SysYParser::CondContext *>(ctx->parent);
    // if (parent && !(parent->op->getText() == "&&" || parent->op->getText() == "||")) {
    //   return genPhiForCond(ctx);
    // }
  } else {  // 比较运算
    auto cfg = cfgs_.back();
    auto blk = cfg->GetLastBlock();
    auto cmp_ir = std::any_cast<IRPtr>(visitRelExp(ctx));
    // TODO: 建立分支语句
    if ((logic_parent && (logic_parent->op->getText() == "&&" || logic_parent->op->getText() == "||"))
        || (stmt_parent && (stmt_parent->IF() != nullptr || stmt_parent->WHILE() != nullptr))) {
      // auto br = std::make_shared<BRIR>(IRTYPE::IR_I1, blk);
      auto br = BRIR::Create(IRTYPE::IR_I1, blk);
      blk->AddStatement(br);
      br->AddOprnd(cmp_ir);
      true_list.emplace_back();
      false_list.emplace_back();
      true_list.back().push_back(br);
      false_list.back().push_back(br);
    }
    return cmp_ir;
  }
  return nullptr;
}

/**
 * op=(LT | GT | LE | GE)
*/
std::any MyVisitor::visitRelExp(SysYParser::CondContext *ctx) {
  auto blk = GetLastBlock();
  IRPtr newIR;  // 新生成的IRStatement
  auto lhs = std::any_cast<IRPtr>(visit(ctx->cond(0)));
  auto rhs = std::any_cast<IRPtr>(visit(ctx->cond(1)));
  // 如果是数组元素进行加减乘除，load一下
  LoadVar(lhs);
  LoadVar(rhs);
  // 先来个0扩展
  ZeroExtend(lhs);
  ZeroExtend(rhs);
  IRTYPE::IRTYPE lType = JudgeRetType(lhs);  // 左边是int还是float还是bool
  IRTYPE::IRTYPE rType = JudgeRetType(rhs);  // 右边是int还是float还是bool

  if (BothConst(lhs->GetIRType(), rhs->GetIRType())) {
    newIR = std::any_cast<IRPtr>(CalcValue(lhs, rhs, ctx->op->getText()));
  } else {
    // 大家都是int
    if ((lType == IRTYPE::IR_I32 || lType == IRTYPE::IR_CONST_INT) &&
        (rType == IRTYPE::IR_I32 || rType == IRTYPE::IR_CONST_INT)) {
      // newIR = std::make_shared<CmpIR>(GetNextOpt(IRTYPE::IR_I32, ctx->op->getText()), IRTYPE::IR_I32);
      newIR = CmpIR::Create(GetNextOpt(IRTYPE::IR_I32, ctx->op->getText()), IRTYPE::IR_I32);
    } else {
      // 转成float
      Int2Float(lhs);
      Int2Float(rhs);
      // newIR = std::make_shared<CmpIR>(GetNextOpt(IRTYPE::IR_FLOAT, ctx->op->getText()), IRTYPE::IR_FLOAT);
      newIR = CmpIR::Create(GetNextOpt(IRTYPE::IR_FLOAT, ctx->op->getText()), IRTYPE::IR_FLOAT);
    } // if

    newIR->AddOprnd(lhs);
    newIR->AddOprnd(rhs);
    blk->AddStatement(newIR);
  }
  return newIR;
}

} // namespace SCNUCC