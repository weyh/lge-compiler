#include "ast.h"

#include <iostream>
#include <string>

namespace lge {

void Type::dump(int indent) const {
  std::string indentStr(indent * 2, ' ');
  std::cout << indentStr << "Type: " << toString() << std::endl;
}

std::string Type::toString() const {
  switch (kind) {
  case INT:
    return "int";
  case FLOAT:
    return "float";
  case CHAR:
    return "char";
  case STR:
    return "str";
  case FUNC: {
    std::string result = "(";
    for (size_t i = 0; i < paramTypes.size(); i++) {
      if (i > 0)
        result += ", ";
      result += paramTypes[i]->toString();
    }
    result += ") -> ";
    result += returnType ? returnType->toString() : "void";
    return result;
  }
  }
  return "unknown";
}

void StringLiteral::dump(int indent) const {
  std::string indentStr(indent * 2, ' ');
  std::cout << indentStr << "StringLiteral: \"" << value << "\"" << std::endl;
}

void IntLiteral::dump(int indent) const {
  std::string indentStr(indent * 2, ' ');
  std::cout << indentStr << "IntLiteral: " << value << std::endl;
}

void FloatLiteral::dump(int indent) const {
  std::string indentStr(indent * 2, ' ');
  std::cout << indentStr << "FloatLiteral: " << value << std::endl;
}

void Identifier::dump(int indent) const {
  std::string indentStr(indent * 2, ' ');
  std::cout << indentStr << "Identifier: " << name << std::endl;
}

void UnaryOp::dump(int indent) const {
  std::string indentStr(indent * 2, ' ');
  std::cout << indentStr << "UnaryOp: ";
  switch (op) {
  case NEG:
    std::cout << "-";
    break;
  }
  std::cout << std::endl;
  operand->dump(indent + 1);
}

void BinaryOp::dump(int indent) const {
  std::string indentStr(indent * 2, ' ');
  std::cout << indentStr << "BinaryOp: ";
  switch (op) {
  case ADD:
    std::cout << "+";
    break;
  case SUB:
    std::cout << "-";
    break;
  case MUL:
    std::cout << "*";
    break;
  case DIV:
    std::cout << "/";
    break;
  case LESS_THAN:
    std::cout << "<";
    break;
  case GREATER_THAN:
    std::cout << ">";
    break;
  case LESS_EQUAL:
    std::cout << "<=";
    break;
  case GREATER_EQUAL:
    std::cout << ">=";
    break;
  case EQUAL_EQUAL:
    std::cout << "==";
    break;
  case NOT_EQUAL:
    std::cout << "!=";
    break;
  }
  std::cout << std::endl;
  left->dump(indent + 1);
  right->dump(indent + 1);
}

void FunctionCall::dump(int indent) const {
  std::string indentStr(indent * 2, ' ');
  std::cout << indentStr << "FunctionCall: " << funcName << std::endl;

  for (const auto &arg : args) {
    arg->dump(indent + 1);
  }
}

void FunctionDef::dump(int indent) const {
  std::string indentStr(indent * 2, ' ');
  std::cout << indentStr << "FunctionDef: " << name << std::endl;

  // Dump return type
  std::cout << indentStr << "  ReturnType:" << std::endl;
  returnType->dump(indent + 2);

  // Dump parameters
  if (!parameters.empty()) {
    std::cout << indentStr << "  Parameters:" << std::endl;
    for (const auto &param : parameters) {
      std::cout << indentStr << "    " << param.name << ": ";
      param.type->dump(0);
    }
  }

  // Dump body
  std::cout << indentStr << "  Body:" << std::endl;
  body->dump(indent + 2);
}

void ConditionalExpression::dump(int indent) const {
  std::string indentStr(indent * 2, ' ');
  std::cout << indentStr << "ConditionalExpression:" << std::endl;
  std::cout << indentStr << " Condition:" << std::endl;
  condition->dump(indent + 2);
  std::cout << indentStr << " Then:" << std::endl;
  thenExpr->dump(indent + 2);
  std::cout << indentStr << " Else:" << std::endl;
  elseExpr->dump(indent + 2);
}

void Program::dump(int indent) const {
  std::string indentStr(indent * 2, ' ');
  std::cout << indentStr << "Program:" << std::endl;

  for (const auto &func : functions) {
    func->dump(indent + 1);
  }
}

} // namespace lge
