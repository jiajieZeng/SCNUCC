//
// Created by 邓实诚 on 2024/5/4.
//

#ifndef SCNUCC_MYVISITOR_H
#define SCNUCC_MYVISITOR_H

// #include "antlr4-runtime.h"
#include "./SysYParserBaseVisitor.h"
#include "./SysYParser.h"
#include "./ControlFlowGraph.h"
#include "./SymbolTable.h"
#include "./IRStatement.h"
#include "./Util.h"
#include <memory>
#include <any>
#include <string>



namespace SCNUCC {

using IRPtr = std::shared_ptr<IRStatement>;

using IRStatementVec = std::vector<std::shared_ptr<IRStatement>>;

using WhileBlocks = std::tuple<std::shared_ptr<Block>, std::shared_ptr<Block>, std::shared_ptr<Block>>; 

class MyVisitor : public SysYParserBaseVisitor {
public:
  /**
  * 构造函数
  */
  MyVisitor();

  auto Adjust() -> void;

  auto ToSSA() const -> std::string;

  /** CompUnit → [ CompUnit ] ( Decl | FuncDef )
   * 遍历CompUnit节点，返回一个包含多个CFG的vector，其中每个CFG代表一个函数或者一个全局变量声明。
   * 如果一个CFG是全局变量声明语句，那么它包含一个或多个DeclBlock。
   * @param ctx
   * @return std::vector<std::shared_ptr<ControlFlowGraph>>
   */
  std::any visitCompUnit(SysYParser::CompUnitContext *ctx) override;

  /** Decl → ConstDecl | VarDecl
   * 遍历Decl节点，返回值有两种可能，分别是：
   * 1. 如果Decl全局变量不是const，那么返回一个CFG，其中包含了一个DeclBlock，表示当前的全局变量声明语句。
   * 2. 如果Decl全局变量是const的，那么返回nullptr，因为const全局变量在运行过程中不会发生变化，所以不需要声明LLVM IR的变量定义语句。
   * @param ctx
   * @return std::shared_ptr<ControlFlowGraph> | nullptr
   */
  std::any visitDecl(SysYParser::DeclContext *ctx) override;

  /** ConstDecl → 'const' BType ConstDef { ',' ConstDef } ';'
   * 遍历ConstDecl节点，将ConstDecl中的常量定义以及对应的初始值加入到符号表中，不需要声明LLVM IR的变量定义语句。
   * 由于是常量定义，所以在之后的语句中如果使用到了这个常量，那么直接使用这个常量的值即可。
   * 比如，
   * const int a = 1;  // 这里将a以及初始值1加入到符号表中
   * int b = a + 1;  // 这里直接使用了a的值计算出了b的值为2
   *
   * 这个函数中需要调用Symbol::SetValue或者Symbol::SetScalerValue在符号表中记录常量的值。
   * 这个过程中需要比较Type::array_shape_与初始值的形状是否能对应上。
   * @param ctx
   * @return
   */
  std::any visitConstDecl(SysYParser::ConstDeclContext *ctx) override;

  /** ConstDef → Ident { '[' ConstExp ']' } '=' ConstInitVal
   * 遍历ConstDef节点，返回一个Symbol，表示当前的常量定义。
   * @param ctx
   * @return Symbol
   */
  std::any visitConstDef(SysYParser::ConstDefContext *ctx) override;

  /** ConstInitVal → ConstExp ｜ '{' [ ConstInitVal { ',' ConstInitVal } ] '}'
   * 遍历ConstInitVal节点，返回一个std::vector<int | float>类型的值。
   * vector.size == array_shape_[0] * ... * array_shape_[n]
   * 或者vector.size == 1，表示这是一个标量。
   * 这里需要注意数组的不规则初始化，比如：
   * int a[2][3] = {{1, 2}, {3, 4, 5}}; // 这里a[0][2]和a[1][0]没有初始化，所以需要补0
   *
   * @param ctx
   * @return std::vector<int | float>
   */
  std::any visitConstInitVal(SysYParser::ConstInitValContext *ctx) override;

  /** VarDecl → BType VarDef { ',' VarDef } ';'
   * 遍历VarDecl节点，返回一个CFG，其中包含了多个DeclBlock，表示当前的全局变量声明语句。
   * 比如，
   * int a = 0, b = 0; // 这个将会返回一个包含两个DeclBlock的CFG
   * @param ctx
   * @return std::shared_ptr<ControlFlowGraph>
   */
  std::any visitVarDecl(SysYParser::VarDeclContext *ctx) override;

  /** VarDef → Ident { '[' ConstExp ']' } | Ident { '[' ConstExp ']' } '=' InitVal
   * 遍历VarDef节点，返回一个DeclBlock，表示当前的全局变量声明语句。
   * 这个函数需要考虑如果变量是一个数组，应该如何生成ArrayDeclIR对象并加入到DeclBlock中。
   * 如果变量是一个标量，那么应该生成一个GblVarDeclIR对象并加入到DeclBlock中。
   * @param ctx
   * @return std::vector<std::shared_ptr<IRStatement>>
   */
  std::any visitVarDef(SysYParser::VarDefContext *ctx) override;

  /** InitVal → Exp | '{' [ InitVal { ',' InitVal } ] '}'
   * 遍历InitVal节点，返回一个std::vector<int | float>类型的值。
   * vector.size == array_shape_[0] * ... * array_shape_[n]
   * 或者vector.size == 1，表示这是一个标量。
   * @param ctx
   * @return std::vector<int | float>
   */
  std::any visitInitVal(SysYParser::InitValContext *ctx) override;

  /** FuncDef → FuncType Ident '(' [FuncFParams] ')' Block
   * 遍历FuncDef节点，返回一个CFG，其中包含了多个Block，表示由当前函数生成的CFG。
   * @param ctx
   * @return std::shared_ptr<ControlFlowGraph>
   */
  std::any visitFuncDef(SysYParser::FuncDefContext *ctx) override;

  /** FuncType → 'void' | 'int' | 'float'
   * 遍历FuncType节点，返回一个Type，表示当前函数的返回值类型。
   * @param ctx
   * @return Type
   */
  std::any visitFuncType(SysYParser::FuncTypeContext *ctx) override;

  /** FuncFParams → FuncFParam { ',' FuncFParam }
   * 遍历FuncFParams节点，返回一个std::vector<Type>，表示当前函数的参数类型。
   * @param ctx
   * @return
   */
  std::any visitFuncFParams(SysYParser::FuncFParamsContext *ctx) override;

  /** FuncFParam → BType Ident ['[' ']' { '[' Exp ']' }]
   * 遍历FuncFParam节点，返回一个Symbol，表示当前函数的一个参数
   * @param ctx
   * @return Symbol
   */
  std::any visitFuncFParam(SysYParser::FuncFParamContext *ctx) override;

  /** Block → '{' { BlockItem } '}'
   * 遍历Block节点，返回一个ControlFlowGraph，表示由当前Block生成的CFG。
   * @param ctx
   * @return std::shared_ptr<ControlFlowGraph>
   */
  std::any visitBlock(SysYParser::BlockContext *ctx) override;

  /** BlockItem → Decl | Stmt
   * 遍历BlockItem节点，返回一个CFG, 表示由当前BlockItem生成的CFG。
   * 需要注意的是，BlockItem有可能是一个Decl，包括常量定义、变量定义以及数组定义，需要谨慎小心处理。
   * @param ctx
   * @return std::shared_ptr<ControlFlowGraph>
   */
  std::any visitBlockItem(SysYParser::BlockItemContext *ctx) override;

  /** Stmt → LVal '=' Exp ';' | [Exp] ';' | Block
   *                  | 'if' '( Cond ')' Stmt [ 'else' Stmt ]
   *                  | 'while' '(' Cond ')' Stmt
   *                  | 'break' ';' | 'continue' ';'
   *                  | 'return' [Exp] ';'
   * 遍历Stmt节点，返回一个CFG, 表示由当前visitStmt生成的CFG。
   * @param ctx
   * @return std::shared_ptr<ControlFlowGraph>
   */
  std::any visitStmt(SysYParser::StmtContext *ctx) override;

  /** Exp → AddExp
   * 遍历Exp节点，返回一个std::vector<std::shared_ptr<IRStatement>>, 表示由当前这个表达式生成的IR语句。
   * 比如，
   * d = a + b + c; // 这里返回一个包含两个AddIR的vector
   * @param ctx
   * @return std::vector<std::shared_ptr<IRStatement>>
   */
  std::any visitExp(SysYParser::ExpContext *ctx) override;

  /** LVal → Ident {'[' Exp ']'}
   * 遍历LVal节点，返回一个Symbol以及索引相对于数组开头的偏移量，表示当前的左值。
   * 比如，
   * a[2][2] = 1; // 这里返回一个Symbol以及一个4，表示a[1][2]这个位置的Symbol以及偏移量。
   *
   * 这个过程中需要检查符号是否定义、数组的越界访问等语法问题。
   * @param ctx
   * @return std::pair<std::shared_ptr<Symbol>, size_t>
   */
  std::any visitLVal(SysYParser::LValContext *ctx) override;

  /** UnaryExp → PrimaryExp | Ident '(' [FuncRParams] ')' | UnaryOp UnaryExp
   * 遍历UnaryExp节点，返回一个std::vector<std::shared_ptr<IRStatement>>, 表示由当前这个一元表达式生成的IR语句。
   * @param ctx
   * @return std::vector<std::shared_ptr<IRStatement>>
   */
  std::any visitUnaryExp(SysYParser::ExpContext *ctx);

  /** number -> intConst | floatConst;
   * 获取Number, 不实现
   * @param ctx
   * @return 
  */
  std::any visitNumber(SysYParser::NumberContext *ctx) override;

  /** MulExp → UnaryExp | MulExp ('*' | '/' | '%') UnaryExp
   * 遍历MulExp节点，返回一个std::vector<std::shared_ptr<IRStatement>>, 表示由当前这个乘法表达式生成的IR语句。
   * @param ctx
   * @return std::vector<std::shared_ptr<IRStatement>>
   */
  std::any visitMulAddExp(SysYParser::ExpContext *ctx);

  /** Cond → LOrExp
   * 遍历Cond节点，返回一个std::vector<IRStatement>, 表示由当前这个条件表达式生成的IR语句。
   * @param ctx
   * @return std::vector<std::shared_ptr<IRStatement>>
   */
  std::any visitCond(SysYParser::CondContext *ctx) override;

  /** RelExp → AddExp | RelExp ('<' | '>' | '<=' | '>=') AddExp
   * 遍历RelExp节点，返回一个std::vector<std::shared_ptr<IRStatement>>, 表示由当前这个关系表达式生成的IR语句。
   * @param ctx
   * @return std::vector<std::shared_ptr<IRStatement>>
   */
  std::any visitRelExp(SysYParser::CondContext *ctx);

  void SyLibInit();  // 初始化运行时静态库函数

  auto GetCFGS() -> std::vector<std::shared_ptr<ControlFlowGraph>>;

private:
  friend class AsmGenerator;
  std::shared_ptr<SymbolTable> sym_table_;
  std::vector<std::shared_ptr<ControlFlowGraph>> cfgs_;
  std::map<std::string, IRPtr> gbl_vars;
  std::shared_ptr<GblInitBlock> gbl_init_blk;
  // 真假出口链和backpatch的设计可以参考https://gitlab.eduxiji.net/T202410574202994/scnucc/-/issues/30
  std::vector<std::vector<std::shared_ptr<BRIR>>> false_list;  // 真出口链
  std::vector<std::vector<std::shared_ptr<BRIR>>> true_list;  // 假出口链
  BLOCK::BLOCK return_flag_; // 这个block有没有被提前return掉
  std::shared_ptr<Block> return_block_;  // ret
  std::vector<WhileBlocks> while_list_; // 当前处于的while循环0:entry 1: body 2:exit

  void backPatch(std::vector<std::shared_ptr<BRIR>> &list, const std::shared_ptr<Block> &block, bool direction);

  IRPtr genPhiForCond(SysYParser::CondContext *ctx);

  void fillFuncParam(SysYParser::FuncDefContext *ctx);

  BaseType string2CType(const std::string &type);

  Type buildType(BaseType type, SymKind kind = SymKind::VAR, bool is_const = false, bool is_global = false,
                 std::vector<size_t> array_shape = {});

  std::shared_ptr<FuncSymbol> buildFuncSymbol(SysYParser::FuncDefContext *ctx);

  std::any string2Number(SysYParser::NumberContext *ctx);

  /**
   * 判断这个语句的返回结果是哪种类型的
   */
  auto JudgeRetType(const IRPtr &IR) -> IRTYPE::IRTYPE;

  auto IsConst(IRTYPE::IRTYPE type) -> int;

  auto BothConst(IRTYPE::IRTYPE lhs, IRTYPE::IRTYPE rhs) -> int;

  template<typename T>
  auto Calculate(T lVal, T rVal, const std::string &op) -> std::pair<T, IRTYPE::IRTYPE>;

  auto CalcValue(const IRPtr &lhs, const IRPtr &rhs, const std::string &op) -> std::any;

  auto GetNextOpt(const IRTYPE::IRTYPE type, const std::string &op) -> IROP::IROP;

  auto ReadVariable(const std::string &var) -> std::any;

  auto WriteVariable(const std::string &var, const IRPtr &stmt) -> void;

  auto ZeroExtend(IRPtr &ir) -> void;

  // 用了的话转换语句的位置会不同
  auto Int2Float(IRPtr &ir) -> void;

  auto Float2Int(IRPtr &IRs) -> void;

  auto Append2Lhs(IRStatementVec &lhs, IRStatementVec &rhs) -> void;

  auto GetLastBlock() -> std::shared_ptr<Block>;

  auto LoadVar(IRPtr &ir) -> void;

  // type是数据的类型
  auto GetArrayInitVal(const size_t &cur, std::shared_ptr<VarSymbol> &sym, SysYParser::ConstInitValContext *ctx) -> std::any;
  
  /**
   * GetArray调用时，constInivalCtx都是直接推导出一个constExp
   * 此时GetArray把每个一个constInitValCtx返回的exp组装成为一个数组，并且返回
   * */
  auto GetArray(const size_t &cur, std::shared_ptr<VarSymbol> &sym,
                        std::vector<SysYParser::ConstInitValContext*>& ctxs, size_t& idx) -> std::vector<std::any>;

  auto GetArrayInitVal(const size_t &cur, std::shared_ptr<VarSymbol> &sym, SysYParser::InitValContext *ctx) -> std::any;

  auto GetArray(const size_t &cur, std::shared_ptr<VarSymbol> &sym, 
                      std::vector<SysYParser::InitValContext*>& ctxs, size_t& idx) -> std::vector<std::any>;

  auto LocalInitArray(const std::shared_ptr<VarSymbol> &symbol, IRPtr &oprnd,
                            IRStatementVec &indices, const std::any &values) -> void;

  auto GlobalInitArray(const std::shared_ptr<VarSymbol> &symbol, 
                        const std::any &values, const int cur,
                        std::stringstream &ssa) -> void;

  template<typename T>
  auto DumpConstArray(std::shared_ptr<VarSymbol> &symbol, std::vector<size_t> &indices, 
                          std::any &values, std::map<int, T> &mp) -> void {
    if (values.type() == typeid(std::vector<std::any>)) {
      // 还套着一层
      auto vt = std::any_cast<std::vector<std::any>> (values);
      for (size_t i = 0; i < vt.size(); i++) {
        indices.emplace_back(i);
        DumpConstArray(symbol, indices, vt[i], mp);
        indices.pop_back();
      }
    } else { // 到达底层
      // 这个一定是ConstantIR
      IRPtr ir = std::any_cast<IRPtr>(values);
      int pos = 0;
      int stride = 1;
      std::vector<size_t> shape = symbol->GetType().array_shape_;
      for (int i = (int)indices.size() - 1; i >= 0; i--) {
        pos += indices[i] * stride;
        stride *= shape[i];
      }
      if (symbol->GetType().base_type_ == BaseType::INT) {
        mp[pos] = std::any_cast<int>(ir->GetConstantVal());
      } else {
        mp[pos] = std::any_cast<float>(ir->GetConstantVal());
      }
    }
  }

};
}


#endif //SCNUCC_MYVISITOR_H
