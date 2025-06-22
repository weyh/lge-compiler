#include <iostream>
#include <memory>

#include <CLI/CLI.hpp>

#include "codegen.h"
#include "lexer.h"
#include "parser.h"

int main(int argc, char **argv) {
  CLI::App app{"LGE"};

  std::string inputFile;
  bool dumpTokens = false, dumpAST = false;

  app.add_option("input_file", inputFile, "Input LGE source file")
      ->required()
      ->check(CLI::ExistingFile);

  app.add_flag("--dump-tokens", dumpTokens, "Dump lexer tokens to stdout");
  app.add_flag("--dump-ast", dumpAST, "Dump AST to stdout");

  CLI11_PARSE(app, argc, argv);

  try {
    /** Lexical analysis **/
    lge::Lexer lexer(inputFile);

    if (dumpTokens) {
      std::cout << "Tokens: " << std::endl;
      lexer.dumpTokens();
      std::cout << "END Tokens" << std::endl;
    }

    /** Parsing **/
    lge::Parser parser(lexer);
    const auto program = parser.parse();

    if (parser.hasErrors()) {
      std::cerr << "Parse errors occurred:" << std::endl;
      parser.printErrors();
      return 1;
    }

    if (dumpAST) {
      std::cout << "AST: " << std::endl;
      parser.dumpAST(*program);
      std::cout << "END AST" << std::endl;
    }

    /** Code generation **/
    lge::CodeGenerator codegen;
    codegen.generate(*program);

    /** Output LLVM IR to stdout **/
    codegen.emitIR();

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
