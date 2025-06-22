#include "codegen.h"

#include <iostream>
#include <sstream>

#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>

namespace lge {

CodeGenerator::CodeGenerator() {
  module = std::make_unique<llvm::Module>("LGE Module", context);
  builder = std::make_unique<llvm::IRBuilder<>>(context);

  declareBuiltinFunctions();
}

void CodeGenerator::generate(const Program &program) {
  for (const auto &func : program.functions) {
    generateFunction(*func);
  }

  // Verify the module
  std::string errorString;
  llvm::raw_string_ostream errorStream(errorString);
  if (llvm::verifyModule(*module, &errorStream)) {
    std::cerr << "Module verification failed: " << errorString << std::endl;
  }
}

void CodeGenerator::emitIR() { module->print(llvm::outs(), nullptr); }

std::string CodeGenerator::getIR() {
  std::string output;
  llvm::raw_string_ostream outputStream(output);
  module->print(outputStream, nullptr);
  return output;
}

llvm::Type *CodeGenerator::llvmType(const Type &type) {
  switch (type.kind) {
  case Type::INT:
    return llvm::Type::getInt32Ty(context);
  case Type::FLOAT:
    return llvm::Type::getFloatTy(context);
  case Type::CHAR:
    return llvm::Type::getInt8Ty(context);
  case Type::STR:
    return llvm::PointerType::get(llvm::Type::getInt8Ty(context),
                                  0); // char*
  case Type::FUNC:
    // In a more sophisticated impl, we would need to
    // track the specific fn signature
    return llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0);
  default:
    reportError("Unknown type", type.location);
    return nullptr;
  }
}

llvm::Value *CodeGenerator::generateExpression(const Expression &expr) {
  // Handle different expr types
  if (const auto *intLit = dynamic_cast<const IntLiteral *>(&expr)) {
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), intLit->value);
  }

  if (const auto *floatLit = dynamic_cast<const FloatLiteral *>(&expr)) {
    return llvm::ConstantFP::get(llvm::Type::getFloatTy(context), floatLit->value);
  }

  if (const auto *strLit = dynamic_cast<const StringLiteral *>(&expr)) {
    return builder->CreateGlobalStringPtr(strLit->value, "str");
  }

  if (const auto *ident = dynamic_cast<const Identifier *>(&expr)) {
    // Look up vars fst
    auto it = namedValues.find(ident->name);
    if (it != namedValues.end()) {
      return it->second;
    }

    // Check if this is a function ref
    auto funcIt = functions.find(ident->name);
    if (funcIt != functions.end()) {
      // Return function ptr
      return builder->CreateBitCast(funcIt->second,
                                    llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0));
    }

    reportError("Undefined variable: " + ident->name, ident->location);
    return nullptr;
  }

  if (const auto *unaryOp = dynamic_cast<const UnaryOp *>(&expr)) {
    llvm::Value *operand = generateExpression(*unaryOp->operand);
    if (!operand)
      return nullptr;

    switch (unaryOp->op) {
    case UnaryOp::NEG:
      if (operand->getType()->isIntegerTy()) {
        return builder->CreateNeg(operand, "negtmp");
      } else if (operand->getType()->isFloatingPointTy()) {
        return builder->CreateFNeg(operand, "fnegtmp");
      }
      break;
    }
  }

  if (const auto *binOp = dynamic_cast<const BinaryOp *>(&expr)) {
    llvm::Value *left = generateExpression(*binOp->left);
    llvm::Value *right = generateExpression(*binOp->right);

    if (!left || !right)
      return nullptr;

    switch (binOp->op) {
    case BinaryOp::ADD:
      if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
        return builder->CreateAdd(left, right, "addtmp");
      } else if (left->getType()->isFloatingPointTy() && right->getType()->isFloatingPointTy()) {
        return builder->CreateFAdd(left, right, "faddtmp");
      }
      break;
    case BinaryOp::SUB:
      if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
        return builder->CreateSub(left, right, "subtmp");
      } else if (left->getType()->isFloatingPointTy() && right->getType()->isFloatingPointTy()) {
        return builder->CreateFSub(left, right, "fsubtmp");
      }
      break;
    case BinaryOp::MUL:
      if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
        return builder->CreateMul(left, right, "multmp");
      } else if (left->getType()->isFloatingPointTy() && right->getType()->isFloatingPointTy()) {
        return builder->CreateFMul(left, right, "fmultmp");
      }
      break;
    case BinaryOp::DIV:
      if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
        return builder->CreateSDiv(left, right, "divtmp");
      } else if (left->getType()->isFloatingPointTy() && right->getType()->isFloatingPointTy()) {
        return builder->CreateFDiv(left, right, "fdivtmp");
      }
    case BinaryOp::LESS_THAN:
      if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
        return builder->CreateICmpSLT(left, right, "cmptmp");
      } else if (left->getType()->isFloatingPointTy() && right->getType()->isFloatingPointTy()) {
        return builder->CreateFCmpOLT(left, right, "cmptmp");
      }
      break;
    case BinaryOp::GREATER_THAN:
      if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
        return builder->CreateICmpSGT(left, right, "cmptmp");
      } else if (left->getType()->isFloatingPointTy() && right->getType()->isFloatingPointTy()) {
        return builder->CreateFCmpOGT(left, right, "cmptmp");
      }
      break;
    case BinaryOp::LESS_EQUAL:
      if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
        return builder->CreateICmpSLE(left, right, "cmptmp");
      } else if (left->getType()->isFloatingPointTy() && right->getType()->isFloatingPointTy()) {
        return builder->CreateFCmpOLE(left, right, "cmptmp");
      }
      break;
    case BinaryOp::GREATER_EQUAL:
      if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
        return builder->CreateICmpSGE(left, right, "cmptmp");
      } else if (left->getType()->isFloatingPointTy() && right->getType()->isFloatingPointTy()) {
        return builder->CreateFCmpOGE(left, right, "cmptmp");
      }
      break;
    case BinaryOp::EQUAL_EQUAL:
      if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
        return builder->CreateICmpEQ(left, right, "cmptmp");
      } else if (left->getType()->isFloatingPointTy() && right->getType()->isFloatingPointTy()) {
        return builder->CreateFCmpOEQ(left, right, "cmptmp");
      }
      break;
    case BinaryOp::NOT_EQUAL:
      if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
        return builder->CreateICmpNE(left, right, "cmptmp");
      } else if (left->getType()->isFloatingPointTy() && right->getType()->isFloatingPointTy()) {
        return builder->CreateFCmpONE(left, right, "cmptmp");
      }
      break;
    }

    reportError("Unsupported binary operation", binOp->location);
    return nullptr;
  }

  if (const auto *call = dynamic_cast<const FunctionCall *>(&expr)) {
    auto namedValueIt = namedValues.find(call->funcName);
    if (namedValueIt != namedValues.end()) {
      // This is a function parameter => create indirect call
      llvm::Value *funcPtr = namedValueIt->second;

      // Generate args
      std::vector<llvm::Value *> args;
      for (const auto &arg : call->args) {
        llvm::Value *argValue = generateExpression(*arg);
        if (!argValue)
          return nullptr;
        args.push_back(argValue);
      }

      // Create function type for indirect call
      std::vector<llvm::Type *> argTypes;
      for (auto *arg : args) {
        argTypes.push_back(arg->getType());
      }

      // Determine ret type based on ctx (assume int)
      llvm::Type *returnType = llvm::Type::getInt32Ty(context);
      llvm::FunctionType *funcType = llvm::FunctionType::get(returnType, argTypes, false);

      // Cast the func ptr and create indirect call
      llvm::Value *castedFunc =
          builder->CreateBitCast(funcPtr, llvm::PointerType::get(funcType, 0));
      return builder->CreateCall(funcType, castedFunc, args, "calltmp");
    }

    llvm::Function *func = nullptr;

    auto it = functions.find(call->funcName);
    if (it != functions.end()) {
      func = it->second;
    } else {
      // Check for built in funx
      func = module->getFunction(call->funcName);
    }

    if (!func) {
      reportError("Undefined function: " + call->funcName, call->location);
      return nullptr;
    }

    if (func->arg_size() != call->args.size()) {
      reportError("Incorrect number of arguments for function: " + call->funcName, call->location);
      return nullptr;
    }

    // Gen args
    std::vector<llvm::Value *> args;
    for (const auto &arg : call->args) {
      llvm::Value *argValue = generateExpression(*arg);
      if (!argValue)
        return nullptr;
      args.push_back(argValue);
    }

    return builder->CreateCall(func, args, "calltmp");
  }

  if (const auto *condExpr = dynamic_cast<const ConditionalExpression *>(&expr)) {
    llvm::Value *condition = generateExpression(*condExpr->condition);
    if (!condition)
      return nullptr;

    // Convert condition to boolean (i1)
    llvm::Value *condBool = nullptr;
    if (condition->getType()->isIntegerTy()) {
      condBool = builder->CreateICmpNE(condition, llvm::ConstantInt::get(condition->getType(), 0),
                                       "ifcond");
    } else if (condition->getType()->isFloatingPointTy()) {
      condBool = builder->CreateFCmpONE(condition, llvm::ConstantFP::get(condition->getType(), 0.0),
                                        "ifcond");
    } else {
      reportError("Invalid condition type for if expression", condExpr->location);
      return nullptr;
    }

    // Get the current function
    llvm::Function *func = builder->GetInsertBlock()->getParent();

    // Create blocks - they are automatically added to the function when created with a function
    // parameter
    llvm::BasicBlock *thenBlock = llvm::BasicBlock::Create(context, "then", func);
    llvm::BasicBlock *elseBlock = llvm::BasicBlock::Create(context, "else", func);
    llvm::BasicBlock *mergeBlock = llvm::BasicBlock::Create(context, "ifcont", func);

    builder->CreateCondBr(condBool, thenBlock, elseBlock);

    // Generate then block
    builder->SetInsertPoint(thenBlock);
    llvm::Value *thenValue = generateExpression(*condExpr->thenExpr);
    if (!thenValue)
      return nullptr;
    builder->CreateBr(mergeBlock);
    thenBlock = builder->GetInsertBlock(); // Update in case of nested expr

    // Generate else block
    builder->SetInsertPoint(elseBlock);
    llvm::Value *elseValue = generateExpression(*condExpr->elseExpr);
    if (!elseValue)
      return nullptr;
    builder->CreateBr(mergeBlock);
    elseBlock = builder->GetInsertBlock(); // Update in case of nested expr

    // Generate merge block
    builder->SetInsertPoint(mergeBlock);

    // Create phi node to merge the values
    llvm::PHINode *phi = builder->CreatePHI(thenValue->getType(), 2, "iftmp");
    phi->addIncoming(thenValue, thenBlock);
    phi->addIncoming(elseValue, elseBlock);

    return phi;
  }

  reportError("Unknown expression type", expr.location);
  return nullptr;
}

llvm::Function *CodeGenerator::generateFunction(const FunctionDef &func) {
  llvm::Type *returnType = llvmType(*func.returnType);

  std::vector<llvm::Type *> paramTypes;
  for (const auto &param : func.parameters) {
    paramTypes.push_back(llvmType(*param.type));
  }

  llvm::FunctionType *funcType = llvm::FunctionType::get(returnType, paramTypes, false);

  llvm::Function *function =
      llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, func.name, module.get());

  // Set arg names
  unsigned idx = 0;
  for (auto &arg : function->args()) {
    arg.setName(func.parameters[idx++].name);
  }

  // Create block
  llvm::BasicBlock *entry = llvm::BasicBlock::Create(context, "entry", function);
  builder->SetInsertPoint(entry);

  currentFunction = function;
  namedValues.clear();

  for (auto &arg : function->args()) {
    namedValues[std::string(arg.getName())] = &arg;
  }

  llvm::Value *retVal = generateExpression(*func.body);
  if (retVal) {
    builder->CreateRet(retVal);

    std::string errorString;
    llvm::raw_string_ostream errorStream(errorString);
    if (llvm::verifyFunction(*function, &errorStream)) {
      std::cerr << "Function verification failed for " << func.name << ": " << errorString
                << std::endl;
    }

    functions[func.name] = function;
    return function;
  }

  // Error occurred => rem func
  function->eraseFromParent();
  return nullptr;
}

void CodeGenerator::declareBuiltinFunctions() {
  // str_print function: (str) -> int
  declareBuiltinFunction("str_print", llvm::Type::getInt32Ty(context),
                         {llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0)});

  // str_read function: (int) -> str
  declareBuiltinFunction("str_read", llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0),
                         {llvm::Type::getInt32Ty(context)});

  // str_len function: (str) -> int
  declareBuiltinFunction("str_len", llvm::Type::getInt32Ty(context),
                         {llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0)});

  // str_at function: (str, int) -> char
  declareBuiltinFunction(
      "str_at", llvm::Type::getInt8Ty(context),
      {llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0), llvm::Type::getInt32Ty(context)});

  // str_sub function: (str, int, int) -> str
  declareBuiltinFunction("str_sub", llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0),
                         {llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0),
                          llvm::Type::getInt32Ty(context), llvm::Type::getInt32Ty(context)});

  // str_find function: (str, str) -> int
  declareBuiltinFunction("str_find", llvm::Type::getInt32Ty(context),
                         {llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0),
                          llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0)});

  // int_to_str function: (int) -> str
  declareBuiltinFunction("int_to_str", llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0),
                         {llvm::Type::getInt32Ty(context)});

  // str_to_int function: (str) -> int
  declareBuiltinFunction("str_to_int", llvm::Type::getInt32Ty(context),
                         {llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0)});

  // float_to_str function: (float) -> str
  declareBuiltinFunction("float_to_str", llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0),
                         {llvm::Type::getFloatTy(context)});

  // str_to_float function: (str) -> float
  declareBuiltinFunction("str_to_float", llvm::Type::getFloatTy(context),
                         {llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0)});

  // str_cmp function: (str, str) -> int
  declareBuiltinFunction("str_cmp", llvm::Type::getInt32Ty(context),
                         {llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0),
                          llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0)});
}

llvm::Function *CodeGenerator::declareBuiltinFunction(const std::string &name,
                                                      llvm::Type *returnType,
                                                      const std::vector<llvm::Type *> &paramTypes) {
  llvm::FunctionType *funcType = llvm::FunctionType::get(returnType, paramTypes, false);
  auto *func =
      llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, name, module.get());
  func->setCallingConv(llvm::CallingConv::C);

  return func;
}

void CodeGenerator::reportError(const std::string &message, const Location &loc) {
  std::cerr << "Code generation error at " << loc.filename << ":" << loc.line << ":" << loc.column
            << ": " << message << std::endl;
}

} // namespace lge
