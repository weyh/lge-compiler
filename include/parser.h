#pragma once

#include <memory>
#include <vector>

#include "ast.h"
#include "lexer.h"

namespace lge {

class Parser {
public:
  Parser(Lexer &lexer);

  std::unique_ptr<Program> parse();

  void dumpAST(const Program &program);
  bool hasErrors() const { return !errors.empty(); }
  void printErrors() const;

private:
  Lexer &lexer;
  std::vector<Token> tokens;
  size_t current = 0;
  std::vector<std::string> errors;

  Token peek() const;
  Token previous() const;
  bool isAtEnd() const;
  Token advance();
  bool check(TokenType type) const;
  bool match(std::initializer_list<TokenType> types);
  Token consume(TokenType type, const std::string &message);
  void synchronize();

  void error(const std::string &message);
  void error(const Token &token, const std::string &message);

  std::unique_ptr<FunctionDef> parseFunction();
  std::unique_ptr<Type> parseType();
  std::vector<Parameter> parseParameters();
  std::unique_ptr<Expression> parseExpression();
  std::unique_ptr<Expression> parseAddition();
  std::unique_ptr<Expression> parseMultiplication();
  std::unique_ptr<Expression> parseUnary();
  std::unique_ptr<Expression> parsePrimary();
  std::unique_ptr<Expression> parseCall(std::unique_ptr<Expression> expr);
  std::unique_ptr<Expression> parseConditional();
  std::unique_ptr<Expression> parseComparison();
};

} // namespace lge
