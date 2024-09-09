#include <filesystem>
#include <fstream>
#include <iostream>

#include "./include/MyVisitor.h"
#include "./include/SysYLexer.h"
#include "./include/SysYParser.h"
#include "ANTLRInputStream.h"
#include "CommonTokenStream.h"
#include "tree/ParseTreeWalker.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }

    std::string input_file = argv[1];
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

    // freopen("debug.txt","w",stderr);

    SCNUCC::MyVisitor visitor;
    visitor.visitCompUnit(tree);
    visitor.Adjust();
    std::cout << visitor.ToSSA() << std::endl;

    return 0;
}