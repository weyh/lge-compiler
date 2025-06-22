#include "parser.h"

#include <iostream>
#include <sstream>

namespace lge {

Parser::Parser(Lexer &lexer) : lexer(lexer) { tokens = lexer.tokenize(); }

std::unique_ptr<Program> Parser::parse() {
  auto prog = std::make_unique<Program>(Location());

  // Parse functions until EOF
  while (!isAtEnd()) {
    // Skip any comment tokens
    while (match({TokenType::COMMENT})) {
    }

    if (isAtEnd())
      break;

    try {
      auto func = parseFunction();
      if (func) {
        prog->functions.push_back(std::move(func));
      }
    } catch (const std::exception &e) {
      error(e.what());
      synchronize();
    }
  }

  return prog;
}

void Parser::dumpAST(const Program &program) { program.dump(); }

void Parser::printErrors() const {
  for (const auto &err : errors) {
    std::cerr << err << std::endl;
  }
}

Token Parser::peek() const { return tokens[current]; }

Token Parser::previous() const { return tokens[current - 1]; }

bool Parser::isAtEnd() const { return peek().type == TokenType::EOF_TOKEN; }

Token Parser::advance() {
  if (!isAtEnd())
    current++;
  return previous();
}

bool Parser::check(TokenType type) const {
  if (isAtEnd())
    return false;
  return peek().type == type;
}

bool Parser::match(std::initializer_list<TokenType> types) {
  for (auto type : types) {
    if (check(type)) {
      advance();
      return true;
    }
  }
  return false;
}

Token Parser::consume(TokenType type, const std::string &message) {
  if (check(type))
    return advance();

  std::stringstream stream;
  stream << message << " at " << peek().location.line << ":" << peek().location.column;
  throw std::runtime_error(stream.str());
}

void Parser::synchronize() {
  advance();

  while (!isAtEnd()) {
    if (previous().type == TokenType::NEWLINE)
      return;

    switch (peek().type) {
    case TokenType::LET:
      return;
    default:
      break;
    }

    advance();
  }
}

void Parser::error(const std::string &message) { errors.push_back(message); }

void Parser::error(const Token &token, const std::string &message) {
  std::stringstream stream;
  if (token.type == TokenType::EOF_TOKEN) {
    stream << "Error at end of file: " << message;
  } else {
    stream << "Error at " << token.location.filename << ":" << token.location.line << ":"
           << token.location.column << " near '" << token.value << "': " << message;
  }
  errors.push_back(stream.str());
}

std::unique_ptr<FunctionDef> Parser::parseFunction() {
  /*
    Expect "let name: type = (param: type, ...) -> expression"
  */

  // Parse "let"
  consume(TokenType::LET, "Expected 'let' at start of function definition");

  // Parse function name
  Token nameToken = consume(TokenType::IDENTIFIER, "Expected function name after 'let'");
  std::string funcName = nameToken.value;

  // Parse ":"
  consume(TokenType::COLON, "Expected ':' after function name");

  // Parse return type
  auto returnType = parseType();

  // Parse "="
  consume(TokenType::EQUALS, "Expected '=' after return type");

  // Parse "("
  consume(TokenType::LPAREN, "Expected '(' for function parameters");

  // Parse parameters
  std::vector<Parameter> parameters = parseParameters();

  // Parse ")"
  consume(TokenType::RPAREN, "Expected ')' after function parameters");

  // Parse "->"
  consume(TokenType::ARROW, "Expected '->' after parameters");

  // Parse function body expression
  auto body = parseExpression();

  return std::make_unique<FunctionDef>(funcName, std::move(returnType), std::move(parameters),
                                       std::move(body), nameToken.location);
}

std::unique_ptr<Type> Parser::parseType() {
  Token typeToken = advance(); // Consume token
  Type::TypeKind kind;

  switch (typeToken.type) {
  case TokenType::TYPE_INT:
    kind = Type::INT;
    break;
  case TokenType::TYPE_FLOAT:
    kind = Type::FLOAT;
    break;
  case TokenType::TYPE_CHAR:
    kind = Type::CHAR;
    break;
  case TokenType::TYPE_STR:
    kind = Type::STR;
    break;
  case TokenType::TYPE_FUNC:
    kind = Type::FUNC;
    break;
  default:
    throw std::runtime_error("Expected type identifier");
  }

  auto type = std::make_unique<Type>(kind, typeToken.location);

  if (kind == Type::FUNC) {
    // TODO: Implement function type parsing (for higher-order functions)
    // Or not :D
  }

  return type;
}

std::vector<Parameter> Parser::parseParameters() {
  std::vector<Parameter> params;

  // Empty parameter list
  if (check(TokenType::RPAREN)) {
    return params;
  }

  do {
    Token paramName = consume(TokenType::IDENTIFIER, "Expected parameter name");
    consume(TokenType::COLON, "Expected ':' after parameter name");
    auto paramType = parseType();

    params.emplace_back(paramName.value, std::move(paramType), paramName.location);
  } while (match({TokenType::COMMA}));

  return params;
}

std::unique_ptr<Expression> Parser::parseExpression() {
  if (match({TokenType::IF})) {
    return parseConditional();
  }
  return parseComparison();
}

std::unique_ptr<Expression> Parser::parseAddition() {
  auto expr = parseMultiplication();

  while (match({TokenType::PLUS, TokenType::MINUS})) {
    Token op = previous();
    auto right = parseMultiplication();

    BinaryOp::OpType opType;
    if (op.type == TokenType::PLUS) {
      opType = BinaryOp::ADD;
    } else {
      opType = BinaryOp::SUB;
    }

    expr = std::make_unique<BinaryOp>(opType, std::move(expr), std::move(right), op.location);
  }

  return expr;
}

std::unique_ptr<Expression> Parser::parseMultiplication() {
  auto expr = parseUnary();

  while (match({TokenType::MULTIPLY, TokenType::DIVIDE})) {
    Token op = previous();
    auto right = parseUnary();

    BinaryOp::OpType opType;
    if (op.type == TokenType::MULTIPLY) {
      opType = BinaryOp::MUL;
    } else {
      opType = BinaryOp::DIV;
    }

    expr = std::make_unique<BinaryOp>(opType, std::move(expr), std::move(right), op.location);
  }

  return expr;
}

std::unique_ptr<Expression> Parser::parseUnary() {
  if (match({TokenType::MINUS})) {
    Token op = previous();
    auto expr = parseUnary(); // Right-associative for multiple unary operators
    return std::make_unique<UnaryOp>(UnaryOp::NEG, std::move(expr), op.location);
  }
  return parsePrimary();
}

std::unique_ptr<Expression> Parser::parsePrimary() {
  // Handle literals
  if (match({TokenType::STRING_LITERAL})) {
    return std::make_unique<StringLiteral>(previous().value, previous().location);
  }

  if (match({TokenType::INT_LITERAL})) {
    int value = std::stoi(previous().value);
    return std::make_unique<IntLiteral>(value, previous().location);
  }

  if (match({TokenType::FLOAT_LITERAL})) {
    float value = std::stof(previous().value);
    return std::make_unique<FloatLiteral>(value, previous().location);
  }

  // Handle identifiers (variable refs or func calls)
  if (match({TokenType::IDENTIFIER})) {
    auto identifier = std::make_unique<Identifier>(previous().value, previous().location);

    // is it func call?
    if (check(TokenType::LPAREN)) {
      return parseCall(std::move(identifier));
    }

    return identifier;
  }

  // Handle parenthesized exprs
  if (match({TokenType::LPAREN})) {
    auto expr = parseExpression();
    consume(TokenType::RPAREN, "Expected ')' after expression");
    return expr;
  }

  throw std::runtime_error("Expected expression");
}

std::unique_ptr<Expression> Parser::parseCall(std::unique_ptr<Expression> expr) {
  if (auto *ident = dynamic_cast<Identifier *>(expr.get())) {
    consume(TokenType::LPAREN, "Expected '(' after function name");

    std::vector<std::unique_ptr<Expression>> arguments;

    // Parse args
    if (!check(TokenType::RPAREN)) {
      do {
        arguments.push_back(parseExpression());
      } while (match({TokenType::COMMA}));
    }

    consume(TokenType::RPAREN, "Expected ')' after arguments");

    return std::make_unique<FunctionCall>(ident->name, std::move(arguments), ident->location);
  }

  throw std::runtime_error("Expected function name before '('");
}

std::unique_ptr<Expression> Parser::parseConditional() {
  auto condition = parseComparison();

  consume(TokenType::THEN, "Expected 'then' after if condition");
  auto thenExpr = parseExpression();

  consume(TokenType::ELSE, "Expected 'else' after then expression");
  auto elseExpr = parseExpression();

  return std::make_unique<ConditionalExpression>(std::move(condition), std::move(thenExpr),
                                                 std::move(elseExpr), condition->location);
}

std::unique_ptr<Expression> Parser::parseComparison() {
  auto expr = parseAddition();

  while (match({TokenType::LESS_THAN, TokenType::GREATER_THAN, TokenType::LESS_EQUAL,
                TokenType::GREATER_EQUAL, TokenType::EQUAL_EQUAL, TokenType::NOT_EQUAL})) {
    Token op = previous();
    auto right = parseAddition();

    BinaryOp::OpType opType;
    switch (op.type) {
    case TokenType::LESS_THAN:
      opType = BinaryOp::LESS_THAN;
      break;
    case TokenType::GREATER_THAN:
      opType = BinaryOp::GREATER_THAN;
      break;
    case TokenType::LESS_EQUAL:
      opType = BinaryOp::LESS_EQUAL;
      break;
    case TokenType::GREATER_EQUAL:
      opType = BinaryOp::GREATER_EQUAL;
      break;
    case TokenType::EQUAL_EQUAL:
      opType = BinaryOp::EQUAL_EQUAL;
      break;
    case TokenType::NOT_EQUAL:
      opType = BinaryOp::NOT_EQUAL;
      break;
    default:
      throw std::runtime_error("Unknown comparison operator");
    }

    expr = std::make_unique<BinaryOp>(opType, std::move(expr), std::move(right), op.location);
  }

  return expr;
}

} // namespace lge
