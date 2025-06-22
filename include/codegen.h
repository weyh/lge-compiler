#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/raw_ostream.h>

#include "ast.h"

namespace lge {

class CodeGenerator {
public:
  CodeGenerator();
  ~CodeGenerator() = default;

  void generate(const Program &program);

  void emitIR();
  std::string getIR();

private:
  // LLVM infra
  llvm::LLVMContext context;
  std::unique_ptr<llvm::Module> module;
  std::unique_ptr<llvm::IRBuilder<>> builder;

  // Symbol tables
  std::unordered_map<std::string, llvm::Value *> namedValues;
  std::unordered_map<std::string, llvm::Function *> functions;

  // Current function being compiled
  llvm::Function *currentFunction = nullptr;

  // Helper
  llvm::Type *llvmType(const Type &type);
  llvm::Value *generateExpression(const Expression &expr);
  llvm::Function *generateFunction(const FunctionDef &func);

  // Built-in func declarations
  void declareBuiltinFunctions();
  llvm::Function *declareBuiltinFunction(const std::string &name, llvm::Type *returnType,
                                         const std::vector<llvm::Type *> &paramTypes);

  void reportError(const std::string &message, const Location &loc);
};

} // namespace lge
