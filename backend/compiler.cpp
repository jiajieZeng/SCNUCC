#include "AsmGenerator.h"
#include <iostream>
#include <fstream>
#include <filesystem>

#include "ANTLRInputStream.h"
#include "CommonTokenStream.h"
#include "tree/ParseTreeWalker.h"
#include "SysYLexer.h"
#include "SysYParser.h"
#include "MyVisitor.h"

int main(int argc, char *argv[]) {
  if (argc < 5) {
    std::cerr << "Usage: " << argv[0] << " -<ouput_file_type> -o" << " <output_file> <input_file>"<< std::endl;
    return 1;
  }
  std::string input_file = argv[4];
  if (!std::filesystem::exists(input_file)) {
    std::cerr << "File not found: " << input_file << std::endl;
    return 1;
  }
  std::fstream file(input_file);
  antlr4::ANTLRInputStream inputStream(file);
  SCNUCC::SysYLexer lexer(&inputStream);
  antlr4::CommonTokenStream tokens(&lexer);
  SCNUCC::SysYParser parser(&tokens);
  auto tree = parser.compUnit();
  SCNUCC::MyVisitor visitor;
  visitor.visitCompUnit(tree);

  visitor.Adjust();
  std::string ssa = visitor.ToSSA();
  
  std::ofstream out_file(argv[3]);
  if (!out_file) {
    std::cerr << "Error opening output file: " << argv[3] << std::endl;
    return 1;
  }
  if (std::string(argv[1]) == "-ll") {
    out_file << ssa;
  } else {
    SCNUCC::AsmGenerator asm_generator(std::make_shared<SCNUCC::MyVisitor>(visitor));
    out_file << asm_generator.GenCode();
  }
  out_file.close();

  return 0;
}