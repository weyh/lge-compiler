#include "lexer.h"

#include <cctype>
#include <iostream>
#include <string_view>

#include <frozen/unordered_map.h>

namespace {
using namespace lge;

constexpr std::string_view toString(const TokenType type) noexcept {
  constexpr frozen::unordered_map<TokenType, std::string_view,
                                  static_cast<int32_t>(TokenType::__LAST)>
      tokenMap = {{TokenType::UNKNOWN, "UNKNOWN"},
                  {TokenType::IDENTIFIER, "IDENTIFIER"},
                  {TokenType::STRING_LITERAL, "STRING_LITERAL"},
                  {TokenType::INT_LITERAL, "INT_LITERAL"},
                  {TokenType::FLOAT_LITERAL, "FLOAT_LITERAL"},
                  {TokenType::LET, "LET"},
                  {TokenType::IF, "IF"},
                  {TokenType::THEN, "THEN"},
                  {TokenType::ELSE, "ELSE"},
                  {TokenType::ARROW, "ARROW"},
                  {TokenType::PLUS, "PLUS"},
                  {TokenType::MINUS, "MINUS"},
                  {TokenType::MULTIPLY, "MULTIPLY"},
                  {TokenType::DIVIDE, "DIVIDE"},
                  {TokenType::EQUALS, "EQUALS"},
                  {TokenType::LESS_THAN, "LESS_THAN"},
                  {TokenType::GREATER_THAN, "GREATER_THAN"},
                  {TokenType::LESS_EQUAL, "LESS_EQUAL"},
                  {TokenType::GREATER_EQUAL, "GREATER_EQUAL"},
                  {TokenType::EQUAL_EQUAL, "EQUAL_EQUAL"},
                  {TokenType::NOT_EQUAL, "NOT_EQUAL"},
                  {TokenType::LPAREN, "LPAREN"},
                  {TokenType::RPAREN, "RPAREN"},
                  {TokenType::COLON, "COLON"},
                  {TokenType::COMMA, "COMMA"},
                  {TokenType::TYPE_INT, "TYPE_INT"},
                  {TokenType::TYPE_FLOAT, "TYPE_FLOAT"},
                  {TokenType::TYPE_CHAR, "TYPE_CHAR"},
                  {TokenType::TYPE_STR, "TYPE_STR"},
                  {TokenType::TYPE_FUNC, "TYPE_FUNC"},
                  {TokenType::NEWLINE, "NEWLINE"},
                  {TokenType::BACKSLASH, "BACKSLASH"},
                  {TokenType::COMMENT, "COMMENT"},
                  {TokenType::EOF_TOKEN, "EOF_TOKEN"}};

  static_assert(tokenMap.size() == static_cast<size_t>(TokenType::__LAST));

  auto it = tokenMap.find(type);
  return (it != tokenMap.end()) ? it->second : "INVALID_TOKEN_TYPE";
}

const std::unordered_map<std::string, TokenType> keywords = {
    {"let", TokenType::LET},        {"if", TokenType::IF},        {"then", TokenType::THEN},
    {"else", TokenType::ELSE},      {"int", TokenType::TYPE_INT}, {"float", TokenType::TYPE_FLOAT},
    {"char", TokenType::TYPE_CHAR}, {"str", TokenType::TYPE_STR}, {"func", TokenType::TYPE_FUNC}};
} // namespace

namespace lge {

Lexer::Lexer(const std::string &filename) : filename(filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Error: Could not open file " << filename << std::endl;
    return;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  input = buffer.str();
}

Lexer::Lexer(const std::string &input, const std::string &filename)
    : input(input), filename(filename) {}

std::vector<Token> Lexer::tokenize() {
  std::vector<Token> tokens;
  Token token = nextToken();

  while (token.type != TokenType::EOF_TOKEN) {
    tokens.push_back(token);
    token = nextToken();
  }

  tokens.push_back(token); // Add EOF
  return tokens;
}

Token Lexer::nextToken() {
  skipWhitespace();

  if (isAtEnd()) {
    return makeToken(TokenType::EOF_TOKEN);
  }

  char c = advance();

  // Handle identifiers and keywords
  if (std::isalpha(c) || c == '_') {
    position--;
    column--;
    return handleIdentifier();
  }

  // Handle numbers
  if (std::isdigit(c)) {
    position--;
    column--;
    return handleNumber();
  }

  // Handle strings
  if (c == '"') {
    return handleString();
  }

  // Handle comments
  if (c == '#') {
    return handleComment();
  }

  // Handle operators
  switch (c) {
  case '(':
    return makeToken(TokenType::LPAREN, "(");
  case ')':
    return makeToken(TokenType::RPAREN, ")");
  case ',':
    return makeToken(TokenType::COMMA, ",");
  case ':':
    return makeToken(TokenType::COLON, ":");
  case '+':
    return makeToken(TokenType::PLUS, "+");
  case '-':
    if (match('>')) {
      return makeToken(TokenType::ARROW, "->");
    }
    return makeToken(TokenType::MINUS, "-");
  case '*':
    return makeToken(TokenType::MULTIPLY, "*");
  case '/':
    return makeToken(TokenType::DIVIDE, "/");
  case '=':
    if (match('=')) {
      return makeToken(TokenType::EQUAL_EQUAL, "==");
    }
    return makeToken(TokenType::EQUALS, "=");
  case '\\':
    return makeToken(TokenType::BACKSLASH, "\\");
  case '\n':
    return makeToken(TokenType::NEWLINE, "\n");
  case '<':
    if (match('=')) {
      return makeToken(TokenType::LESS_EQUAL, "<=");
    }
    return makeToken(TokenType::LESS_THAN, "<");
  case '>':
    if (match('=')) {
      return makeToken(TokenType::GREATER_EQUAL, ">=");
    }
    return makeToken(TokenType::GREATER_THAN, ">");
  case '!':
    if (match('=')) {
      return makeToken(TokenType::NOT_EQUAL, "!=");
    }
    return errorToken("Unexpected character '!'");
  }

  return errorToken("Unexpected character");
}

void Lexer::dumpTokens() {
  std::vector<Token> tokens = tokenize();
  std::cout << "Tokens for file: " << filename << std::endl;
  std::cout << "=====================================" << std::endl;

  for (const auto &token : tokens) {
    std::cout << "Line " << token.location.line << ", Col " << token.location.column << ": ";
    std::cout << toString(token.type);
    std::cout << " '" << token.value << "'" << std::endl;
  }

  std::cout << "=====================================" << std::endl;
  std::cout << "Total tokens: " << tokens.size() << std::endl;
}

char Lexer::peek(size_t offset) const {
  if (position + offset >= input.size()) {
    return '\0';
  }
  return input[position + offset];
}

char Lexer::advance() {
  if (isAtEnd())
    return '\0';

  char c = input[position++];
  column++;

  if (c == '\n') {
    line++;
    column = 1;
  }

  return c;
}

bool Lexer::match(const char expected) {
  if (isAtEnd() || peek() != expected) {
    return false;
  }

  advance();
  return true;
}

bool Lexer::isAtEnd() const { return position >= input.size(); }

void Lexer::skipWhitespace() {
  while (true) {
    char c = peek();

    switch (c) {
    case ' ':
    case '\t':
    case '\r':
      advance();
      break;

    case '\n':
      advance();
      line++;
      column = 1;
      break;

    default:
      return;
    }
  }
}

Token Lexer::handleIdentifier() {
  const size_t start = position;
  const size_t startColumn = column;

  // 1st char already checked for alpha or underscore
  advance();

  // Rest of the identifier
  while (std::isalnum(peek()) || peek() == '_') {
    advance();
  }

  // Get identifier
  const std::string text = input.substr(start, position - start);

  // Check if for keyword
  TokenType type = TokenType::IDENTIFIER;
  if (keywords.find(text) != keywords.end()) {
    type = keywords.at(text);
  }

  return Token(type, text, Location(line, startColumn, filename));
}

Token Lexer::handleNumber() {
  const size_t start = position;
  const size_t startColumn = column;
  bool isFloat = false;

  // Int part
  while (std::isdigit(peek())) {
    advance();
  }

  // Decimal part
  if (peek() == '.' && std::isdigit(peek(1))) {
    isFloat = true;
    advance(); // Consume '.'

    while (std::isdigit(peek())) {
      advance();
    }
  }

  const std::string numberStr = input.substr(start, position - start);

  if (isFloat) {
    return Token(TokenType::FLOAT_LITERAL, numberStr, Location(line, startColumn, filename));
  } else {
    return Token(TokenType::INT_LITERAL, numberStr, Location(line, startColumn, filename));
  }
}

Token Lexer::handleString() {
  const size_t startColumn = column - 1; // Account for opening quote
  std::string value;

  // Read until closing quote
  while (peek() != '"' && !isAtEnd()) {
    if (peek() == '\n') {
      line++;
      column = 1;
    }

    if (peek() == '\\') {
      advance(); // Consume backslash

      switch (peek()) {
      case '"':
        value += '"';
        break;
      case '\\':
        value += '\\';
        break;
      case 'n':
        value += '\n';
        break;
      case 't':
        value += '\t';
        break;
      case 'r':
        value += '\r';
        break;
      default:
        value += peek();
        break;
      }

      advance();
    } else {
      value += advance();
    }
  }

  if (isAtEnd()) {
    return errorToken("Unterminated string");
  }

  // Consume closing quote
  advance();

  return Token(TokenType::STRING_LITERAL, value, Location(line, startColumn, filename));
}

Token Lexer::handleComment() {
  const size_t startColumn = column - 1; // Account for '#'
  std::string comment = "#";

  // Read until end of line or end of file
  while (peek() != '\n' && !isAtEnd()) {
    comment += advance();
  }

  return Token(TokenType::COMMENT, comment, Location(line, startColumn, filename));
}

Token Lexer::makeToken(const TokenType type, const std::string &value) {
  return Token(type, value, Location(line, column - value.length(), filename));
}

Token Lexer::errorToken(const std::string &message) {
  return Token(TokenType::UNKNOWN, message, Location(line, column, filename));
}

} // namespace lge
