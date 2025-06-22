#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace lge {

enum class TokenType : int32_t {
  UNKNOWN,

  // Literals
  IDENTIFIER,
  STRING_LITERAL,
  INT_LITERAL,
  FLOAT_LITERAL,

  // Keywords
  LET,
  // Conditional keywords
  IF,
  THEN,
  ELSE,

  // Operators
  ARROW,    // ->
  PLUS,     // +
  MINUS,    // -
  MULTIPLY, // *
  DIVIDE,   // /
  EQUALS,   // =

  // Comparison operators
  LESS_THAN,     // <
  GREATER_THAN,  // >
  LESS_EQUAL,    // <=
  GREATER_EQUAL, // >=
  EQUAL_EQUAL,   // ==
  NOT_EQUAL,     // !=

  // Delimiters
  LPAREN, // (
  RPAREN, // )
  COLON,  // :
  COMMA,  // ,

  // Types
  TYPE_INT,   // int
  TYPE_FLOAT, // float
  TYPE_CHAR,  // char
  TYPE_STR,   // str
  TYPE_FUNC,  // func

  // Special
  NEWLINE,
  BACKSLASH, // \ (line continuation)
  COMMENT,   // #
  EOF_TOKEN,

  __LAST
};

struct Location {
  size_t line;
  size_t column;
  std::string filename;

  Location(size_t l = 1, size_t c = 1, const std::string &f = "")
      : line(l), column(c), filename(f) {}
};

struct Token {
  TokenType type;
  std::string value;
  Location location;

  Token(TokenType t, const std::string &v, const Location &loc)
      : type(t), value(v), location(loc) {}
};

class ASTNode;
class Expression;
class FunctionDef;
class Type;

using ASTNodePtr = std::unique_ptr<ASTNode>;
using ExprPtr = std::unique_ptr<Expression>;
using FuncDefPtr = std::unique_ptr<FunctionDef>;
using TypePtr = std::unique_ptr<Type>;

// Base AST node
class ASTNode {
public:
  Location location;

  ASTNode(const Location &loc) : location(loc) {}
  virtual ~ASTNode() = default;
  virtual void dump(int indent = 0) const = 0;
};

class Type : public ASTNode {
public:
  enum TypeKind { INT, FLOAT, CHAR, STR, FUNC };

  TypeKind kind;
  std::vector<TypePtr> paramTypes; // For func types
  TypePtr returnType;              // For func types

  Type(TypeKind k, const Location &loc) : ASTNode(loc), kind(k) {}

  void dump(int indent = 0) const override;
  std::string toString() const;
};

// Base expression class
class Expression : public ASTNode {
public:
  Expression(const Location &loc) : ASTNode(loc) {}
  virtual ~Expression() = default;
};

class StringLiteral : public Expression {
public:
  std::string value;

  StringLiteral(const std::string &val, const Location &loc) : Expression(loc), value(val) {}

  void dump(int indent = 0) const override;
};

class IntLiteral : public Expression {
public:
  int value;

  IntLiteral(int val, const Location &loc) : Expression(loc), value(val) {}

  void dump(int indent = 0) const override;
};

class FloatLiteral : public Expression {
public:
  float value;

  FloatLiteral(float val, const Location &loc) : Expression(loc), value(val) {}

  void dump(int indent = 0) const override;
};

class Identifier : public Expression {
public:
  std::string name;

  Identifier(const std::string &n, const Location &loc) : Expression(loc), name(n) {}

  void dump(int indent = 0) const override;
};

class BinaryOp : public Expression {
public:
  enum OpType {
    ADD,
    SUB,
    MUL,
    DIV,
    LESS_THAN,
    GREATER_THAN,
    LESS_EQUAL,
    GREATER_EQUAL,
    EQUAL_EQUAL,
    NOT_EQUAL
  };

  OpType op;
  ExprPtr left;
  ExprPtr right;

  BinaryOp(OpType o, ExprPtr l, ExprPtr r, const Location &loc)
      : Expression(loc), op(o), left(std::move(l)), right(std::move(r)) {}

  void dump(int indent = 0) const override;
};

class UnaryOp : public Expression {
public:
  enum OpType { NEG };
  OpType op;
  ExprPtr operand;

  UnaryOp(OpType o, ExprPtr operand, const Location &loc)
      : Expression(loc), op(o), operand(std::move(operand)) {}

  void dump(int indent = 0) const override;
};

class FunctionCall : public Expression {
public:
  std::string funcName;
  std::vector<ExprPtr> args;

  FunctionCall(const std::string &name, std::vector<ExprPtr> arguments, const Location &loc)
      : Expression(loc), funcName(name), args(std::move(arguments)) {}

  void dump(int indent = 0) const override;
};

class ConditionalExpression : public Expression {
public:
  ExprPtr condition;
  ExprPtr thenExpr;
  ExprPtr elseExpr;

  ConditionalExpression(ExprPtr cond, ExprPtr thenE, ExprPtr elseE, const Location &loc)
      : Expression(loc), condition(std::move(cond)), thenExpr(std::move(thenE)),
        elseExpr(std::move(elseE)) {}

  void dump(int indent = 0) const override;
};

// Param for func definition
struct Parameter {
  std::string name;
  TypePtr type;
  Location location;

  Parameter(const std::string &n, TypePtr t, const Location &loc)
      : name(n), type(std::move(t)), location(loc) {}
};

// Function def
class FunctionDef : public ASTNode {
public:
  std::string name;
  TypePtr returnType;
  std::vector<Parameter> parameters;
  ExprPtr body;

  FunctionDef(const std::string &n, TypePtr retType, std::vector<Parameter> params, ExprPtr b,
              const Location &loc)
      : ASTNode(loc), name(n), returnType(std::move(retType)), parameters(std::move(params)),
        body(std::move(b)) {}

  void dump(int indent = 0) const override;
};

class Program : public ASTNode {
public:
  std::vector<FuncDefPtr> functions;

  Program(const Location &loc) : ASTNode(loc) {}

  void dump(int indent = 0) const override;
};

} // namespace lge
