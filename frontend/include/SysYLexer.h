
// Generated from SysYLexer.g4 by ANTLR 4.13.1

#pragma once


#include "antlr4-runtime.h"
#include <any>

namespace SCNUCC {

class  SysYLexer : public antlr4::Lexer {
public:
  enum {
    ADD = 1, SUB = 2, MUL = 3, DIV = 4, MOD = 5, EQ = 6, NEQ = 7, LT = 8, 
    GT = 9, LE = 10, GE = 11, AND = 12, OR = 13, NOT = 14, LPAREN = 15, 
    RPAREN = 16, LBRACE = 17, RBRACE = 18, LBRACK = 19, RBRACK = 20, COMMA = 21, 
    SEMICOLON = 22, ASSIGN = 23, DOT = 24, CONST = 25, VOID = 26, INT = 27, 
    FLOAT = 28, IF = 29, ELSE = 30, WHILE = 31, BREAK = 32, CONTINUE = 33, 
    RETURN = 34, IDENTIFIER = 35, DECIMAL_CONST = 36, OCTAL_CONST = 37, 
    HEX_CONST = 38, DECIMAL_FLOATING_CONST = 39, HEX_FLOATING_CONST = 40, 
    WS = 41, SINGLE_LINE_COMMENT = 42, MULTI_LINE_COMMENT = 43, STRING = 44
  };

  explicit SysYLexer(antlr4::CharStream *input);

  ~SysYLexer() override;


  std::string getGrammarFileName() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const std::vector<std::string>& getChannelNames() const override;

  const std::vector<std::string>& getModeNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;

  const antlr4::atn::ATN& getATN() const override;

  // By default the static state used to implement the lexer is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:

  // Individual action functions triggered by action() above.

  // Individual semantic predicate functions triggered by sempred() above.

};

}