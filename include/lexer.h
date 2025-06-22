#pragma once

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "ast.h"

namespace lge {

class Lexer {
public:
  Lexer(const std::string &filename);
  Lexer(const std::string &input, const std::string &filename);

  std::vector<Token> tokenize();
  Token nextToken();

  void dumpTokens();

private:
  std::string input;
  size_t position = 0;
  size_t line = 1;
  size_t column = 1;
  std::string filename;

  char peek(size_t offset = 0) const;
  char advance();
  bool match(const char expected);
  bool isAtEnd() const;
  void skipWhitespace();
  Token handleIdentifier();
  Token handleNumber();
  Token handleString();
  Token handleComment();

  Token makeToken(const TokenType type, const std::string &value = "");
  Token errorToken(const std::string &message);
};

} // namespace lge
