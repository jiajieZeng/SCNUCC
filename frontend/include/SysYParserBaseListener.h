
// Generated from SysYParser.g4 by ANTLR 4.13.1

#pragma once


#include "antlr4-runtime.h"
#include "./SysYParserListener.h"
#include <any>


namespace SCNUCC {

/**
 * This class provides an empty implementation of SysYParserListener,
 * which can be extended to create a listener which only needs to handle a subset
 * of the available methods.
 */
class  SysYParserBaseListener : public SysYParserListener {
public:

  virtual void enterCompUnit(SysYParser::CompUnitContext * /*ctx*/) override { }
  virtual void exitCompUnit(SysYParser::CompUnitContext * /*ctx*/) override { }

  virtual void enterDecl(SysYParser::DeclContext * /*ctx*/) override { }
  virtual void exitDecl(SysYParser::DeclContext * /*ctx*/) override { }

  virtual void enterConstDecl(SysYParser::ConstDeclContext * /*ctx*/) override { }
  virtual void exitConstDecl(SysYParser::ConstDeclContext * /*ctx*/) override { }

  virtual void enterBType(SysYParser::BTypeContext * /*ctx*/) override { }
  virtual void exitBType(SysYParser::BTypeContext * /*ctx*/) override { }

  virtual void enterConstDef(SysYParser::ConstDefContext * /*ctx*/) override { }
  virtual void exitConstDef(SysYParser::ConstDefContext * /*ctx*/) override { }

  virtual void enterConstInitVal(SysYParser::ConstInitValContext * /*ctx*/) override { }
  virtual void exitConstInitVal(SysYParser::ConstInitValContext * /*ctx*/) override { }

  virtual void enterVarDecl(SysYParser::VarDeclContext * /*ctx*/) override { }
  virtual void exitVarDecl(SysYParser::VarDeclContext * /*ctx*/) override { }

  virtual void enterVarDef(SysYParser::VarDefContext * /*ctx*/) override { }
  virtual void exitVarDef(SysYParser::VarDefContext * /*ctx*/) override { }

  virtual void enterInitVal(SysYParser::InitValContext * /*ctx*/) override { }
  virtual void exitInitVal(SysYParser::InitValContext * /*ctx*/) override { }

  virtual void enterFuncDef(SysYParser::FuncDefContext * /*ctx*/) override { }
  virtual void exitFuncDef(SysYParser::FuncDefContext * /*ctx*/) override { }

  virtual void enterFuncType(SysYParser::FuncTypeContext * /*ctx*/) override { }
  virtual void exitFuncType(SysYParser::FuncTypeContext * /*ctx*/) override { }

  virtual void enterFuncFParams(SysYParser::FuncFParamsContext * /*ctx*/) override { }
  virtual void exitFuncFParams(SysYParser::FuncFParamsContext * /*ctx*/) override { }

  virtual void enterFuncFParam(SysYParser::FuncFParamContext * /*ctx*/) override { }
  virtual void exitFuncFParam(SysYParser::FuncFParamContext * /*ctx*/) override { }

  virtual void enterBlock(SysYParser::BlockContext * /*ctx*/) override { }
  virtual void exitBlock(SysYParser::BlockContext * /*ctx*/) override { }

  virtual void enterBlockItem(SysYParser::BlockItemContext * /*ctx*/) override { }
  virtual void exitBlockItem(SysYParser::BlockItemContext * /*ctx*/) override { }

  virtual void enterStmt(SysYParser::StmtContext * /*ctx*/) override { }
  virtual void exitStmt(SysYParser::StmtContext * /*ctx*/) override { }

  virtual void enterConstExp(SysYParser::ConstExpContext * /*ctx*/) override { }
  virtual void exitConstExp(SysYParser::ConstExpContext * /*ctx*/) override { }

  virtual void enterExp(SysYParser::ExpContext * /*ctx*/) override { }
  virtual void exitExp(SysYParser::ExpContext * /*ctx*/) override { }

  virtual void enterCond(SysYParser::CondContext * /*ctx*/) override { }
  virtual void exitCond(SysYParser::CondContext * /*ctx*/) override { }

  virtual void enterLVal(SysYParser::LValContext * /*ctx*/) override { }
  virtual void exitLVal(SysYParser::LValContext * /*ctx*/) override { }

  virtual void enterNumber(SysYParser::NumberContext * /*ctx*/) override { }
  virtual void exitNumber(SysYParser::NumberContext * /*ctx*/) override { }

  virtual void enterIntConst(SysYParser::IntConstContext * /*ctx*/) override { }
  virtual void exitIntConst(SysYParser::IntConstContext * /*ctx*/) override { }

  virtual void enterFloatConst(SysYParser::FloatConstContext * /*ctx*/) override { }
  virtual void exitFloatConst(SysYParser::FloatConstContext * /*ctx*/) override { }

  virtual void enterUnaryOp(SysYParser::UnaryOpContext * /*ctx*/) override { }
  virtual void exitUnaryOp(SysYParser::UnaryOpContext * /*ctx*/) override { }

  virtual void enterFuncRParams(SysYParser::FuncRParamsContext * /*ctx*/) override { }
  virtual void exitFuncRParams(SysYParser::FuncRParamsContext * /*ctx*/) override { }

  virtual void enterString(SysYParser::StringContext * /*ctx*/) override { }
  virtual void exitString(SysYParser::StringContext * /*ctx*/) override { }

  virtual void enterIdent(SysYParser::IdentContext * /*ctx*/) override { }
  virtual void exitIdent(SysYParser::IdentContext * /*ctx*/) override { }


  virtual void enterEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void exitEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void visitTerminal(antlr4::tree::TerminalNode * /*node*/) override { }
  virtual void visitErrorNode(antlr4::tree::ErrorNode * /*node*/) override { }

};

}