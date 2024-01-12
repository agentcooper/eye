#pragma once

#include "Debug.hpp"
#include "LLVM.hpp"
#include "Node.hpp"
#include "SymbolTable.hpp"
#include "Vector.hpp"

#include <iostream>
#include <map>
#include <ranges>

struct LLVMTypeVisitor {
  llvm::LLVMContext &context;
  SymbolTableVisitor &symbolTableVisitor;

  bool functionAsPointer;
  llvm::Type *env;

  llvm::Type *operator()(const PrimitiveType &type) const {
    switch (type) {
    case PrimitiveType::i64Type:
      return llvm::Type::getInt64Ty(context);
    case PrimitiveType::f64Type:
      return llvm::Type::getDoubleTy(context);
    case PrimitiveType::booleanType:
      return llvm::Type::getInt1Ty(context);
    case PrimitiveType::charType:
      return llvm::Type::getInt8Ty(context);
    case PrimitiveType::stringType:
      return llvm::Type::getInt8PtrTy(context);
    case PrimitiveType::voidType:
      return llvm::Type::getVoidTy(context);
    case PrimitiveType::unknownType:
      throw std::runtime_error("Can't get LLVM type for unknown type");
    }
  }

  llvm::Type *operator()(const PointerType &type) const {
    return llvm::Type::getInt8PtrTy(context);
  }

  llvm::Type *operator()(const ArrayType &type) const {
    return llvm::ArrayType::get(std::visit(*this, *type.type), type.size);
  }

  llvm::Type *operator()(const TypeReference &type) const {
    auto symbol = symbolTableVisitor.globalScope.lookup(type.name);
    if (!symbol) {
      throw std::runtime_error("Bad lookup " + type.name);
    }
    return std::visit(*this, *symbol->type);
  }

  llvm::Type *operator()(const StructType &type) const {
    LLVMTypeVisitor visitor{context, symbolTableVisitor, true, env};
    std::vector<llvm::Type *> types{};
    for (const auto &property : type.properties) {
      types.push_back(std::visit(visitor, *property.second));
    }
    return llvm::StructType::get(context, types, false);
  }

  llvm::Type *operator()(const FunctionType &type) const {
    LLVMTypeVisitor visitor{context, symbolTableVisitor, true, env};

    auto returnType = std::visit(visitor, *type.returnType);
    std::vector<llvm::Type *> parameters{};
    if (env != nullptr) {
      parameters.push_back(env);
    }
    for (const auto &parameter : type.parameters) {
      parameters.push_back(std::visit(visitor, *parameter));
    }
    auto functionType = llvm::FunctionType::get(returnType, parameters, false);
    if (functionAsPointer) {
      return functionType->getPointerTo();
    }
    return functionType;
  }
};

class Codegen : public Visitor {
  SourceFileNode &sourceFile;
  SymbolTableVisitor &symbolTableVisitor;

  std::unique_ptr<llvm::LLVMContext> llvmContext;
  std::unique_ptr<llvm::Module> llvmModule;
  std::unique_ptr<llvm::IRBuilder<>> builder;

  llvm::Value *value;
  bool isWrite = false;
  std::map<std::string, llvm::AllocaInst *> namedValues;

  llvm::Type *int64Type;
  llvm::Type *float64Type;
  llvm::Type *voidPointerType;

  size_t arrowFunctionExpressionIndex = 0;
  size_t forScopeIndex = 0;

  std::map<Scope *, llvm::AllocaInst *> scopeEnv;

private:
  llvm::AllocaInst *createEntryBlockAlloca(llvm::Function *function,
                                           llvm::StringRef name,
                                           llvm::Type *type) {
    llvm::IRBuilder<> TmpB(&function->getEntryBlock(),
                           function->getEntryBlock().begin());
    return TmpB.CreateAlloca(type, nullptr, name);
  }

  llvm::Value *allocateClosure(llvm::Function *f,
                               std::vector<Symbol> capturedSymbols,
                               Scope *scope) {
    debug << "Allocating closure" << std::endl;

    auto envType = buildEnvTypeForSymbols(capturedSymbols);
    auto closureType = getClosureType();

    llvm::AllocaInst *envAlloca;

    if (!capturedSymbols.empty()) {
      int envSize = llvmModule->getDataLayout().getTypeAllocSize(envType);
      envAlloca = scopeEnv.contains(scope)
                      ? scopeEnv[scope]
                      : createAllocateCall(envType, "env", envSize);
      scopeEnv[scope] = envAlloca;

      auto env = builder->CreateLoad(envType->getPointerTo(), envAlloca, "env");

      for (size_t idx = 0; const auto &symbol : capturedSymbols) {
        if (namedValues.contains(symbol.name)) {
          debug << "Storing " << symbol.name << " at index " << idx
                << std::endl;
          auto gep = builder->CreateStructGEP(envType, env, idx);
          auto value = builder->CreateLoad(buildLLVMType(*symbol.type),
                                           namedValues[symbol.name],
                                           "env_" + symbol.name);
          builder->CreateStore(value, gep);
        }
        idx += 1;
      }
    }

    auto closureAlloca = createAllocateCall(closureType, "closure", 16);
    auto closure =
        builder->CreateLoad(closureType->getPointerTo(), closureAlloca, "xyz");
    builder->CreateStore(f, builder->CreateStructGEP(closureType, closure, 0));

    if (!capturedSymbols.empty()) {
      builder->CreateStore(
          builder->CreateLoad(envType->getPointerTo(), envAlloca, "zzz"),
          builder->CreateStructGEP(closureType, closure, 1));
    }

    return closure;
  }

  llvm::Type *buildEnvTypeForSymbols(const std::vector<Symbol> &symbols) const {
    std::vector<llvm::Type *> types;
    std::transform(
        symbols.cbegin(), symbols.cend(), std::back_inserter(types),
        [&](const Symbol &symbol) { return buildLLVMType(*symbol.type); });
    return llvm::StructType::get(*llvmContext, types, false);
  }

  llvm::Value *localOrEnv(const std::string &name, bool isRef) const {
    bool isLocal = true;
    auto symbol = symbolTableVisitor.currentScope->lookup(name, false);
    if (!symbol.has_value()) {
      isLocal = false;

      auto p = symbolTableVisitor.currentScope->lookupWithScope(name);
      if (p.has_value()) {
        symbol = (*p).first;
        (*p).second->symbolTable.setCaptured(name, true);
      }
    }
    if (!symbol) {
      throw std::runtime_error("Could not find the symbol '" + name + "'");
    }
    auto llvmType = buildLLVMType(*symbol->type);
    // std::cout << name << " isCaptured: " << symbol->isCaptured << std::endl;
    if (isLocal && symbol->isCaptured && namedValues.contains("env") &&
        scopeEnv.contains(symbolTableVisitor.currentScope)) {
      // std::cout << "get from captured: " << name << std::endl;

      auto envSymbols = symbolTableVisitor.currentScope->symbolTable.getAll(
          [](auto symbol) { return symbol.isCaptured; });
      auto envType = buildEnvTypeForSymbols(envSymbols);

      auto s = symbolTableVisitor.currentScope;

      auto env =
          builder->CreateLoad(envType->getPointerTo(), scopeEnv.at(s), "mmm");
      auto index = find_index(
          envSymbols, [&](auto &symbol) { return symbol.name == name; });
      assert(index != -1);

      auto gep = builder->CreateStructGEP(envType, env, index, "kkl");
      return isRef ? gep : builder->CreateLoad(llvmType, gep, "env_" + name);
    }

    // TODO: improve look-up for non-closure symbols from upper scopes
    if (isLocal || namedValues.contains(name)) {
      auto V = namedValues.at(name);
      if (V == nullptr) {
        throw std::runtime_error("Can't find value: " + name);
      }
      if (isRef) {
        return V;
      }
      return builder->CreateLoad(llvmType, V, name.c_str());
    }

    auto parentScope = symbolTableVisitor.currentScope->parent;

    auto envSymbols = parentScope->symbolTable.getAll(
        [](auto symbol) { return symbol.isCaptured; });
    auto envType = buildEnvTypeForSymbols(envSymbols);
    auto index = find_index(envSymbols,
                            [&](auto &symbol) { return symbol.name == name; });
    assert(index != -1);

    debug << "Index of " << name << " is " << index << std::endl;

    auto env = builder->CreateLoad(envType->getPointerTo(),
                                   namedValues.at("env"), "mmm");
    auto gep = builder->CreateStructGEP(envType, env, index, "kkl");
    return isRef ? gep : builder->CreateLoad(llvmType, gep, "env_" + name);
  }

  llvm::Type *buildLLVMType(const Type &type, llvm::Type *env = nullptr) const {
    LLVMTypeVisitor llvmTypeVisitor{*llvmContext, symbolTableVisitor, true,
                                    env};
    return std::visit(llvmTypeVisitor, type);
  }

  llvm::FunctionType *asFunctionType(const Type &type, llvm::Type *env) const {
    LLVMTypeVisitor llvmTypeVisitor{*llvmContext, symbolTableVisitor, false,
                                    env};
    auto llvmType = std::visit(llvmTypeVisitor, type);
    if (llvm::FunctionType *ft = llvm::dyn_cast<llvm::FunctionType>(llvmType)) {
      return ft;
    }
    throw std::runtime_error("Can't build function type");
  }

  llvm::AllocaInst *createAllocateCall(llvm::Type *type,
                                       const std::string &name, int size) {
    llvm::Function *TheFunction = builder->GetInsertBlock()->getParent();
    auto alloca =
        createEntryBlockAlloca(TheFunction, name, type->getPointerTo());
    auto call = builder->CreateCall(llvmModule->getFunction("__allocate"),
                                    {llvm::ConstantInt::get(int64Type, size)});
    builder->CreateStore(call, alloca);
    return alloca;
  }

  llvm::StructType *getClosureType() const {
    return llvm::StructType::get(*llvmContext,
                                 {voidPointerType, voidPointerType}, false);
  }

  void create_beforeStart_call() {
    auto functionType =
        llvm::FunctionType::get(llvm::Type::getVoidTy(*llvmContext), {}, false);
    llvm::Function::Create(functionType, llvm::Function::ExternalLinkage,
                           "beforeStart", llvmModule.get());
  }

  void create_beforeExit_call() {
    auto functionType =
        llvm::FunctionType::get(llvm::Type::getVoidTy(*llvmContext), {}, false);
    llvm::Function::Create(functionType, llvm::Function::ExternalLinkage,
                           "beforeExit", llvmModule.get());
  }

  void createAllocateFunction() {
    auto returnType =
        llvm::FunctionType::get(int64Type->getPointerTo(), {int64Type}, false);
    llvm::Function::Create(returnType, llvm::Function::ExternalLinkage,
                           "__allocate", llvmModule.get());
  }

public:
  Codegen(SourceFileNode &sourceFile, SymbolTableVisitor &symbolTableVisitor)
      : sourceFile(sourceFile), symbolTableVisitor(symbolTableVisitor) {
    llvmContext = std::make_unique<llvm::LLVMContext>();
    llvmModule = std::make_unique<llvm::Module>("SourceFile", *llvmContext);
    builder = std::make_unique<llvm::IRBuilder<>>(*llvmContext);

    int64Type = llvm::IntegerType::get(*llvmContext, 64);
    float64Type = llvm::Type::getDoubleTy(*llvmContext);
    voidPointerType = llvm::Type::getInt64PtrTy(*llvmContext);
  };

  void visit(TypeReferenceNode &node) override {}

  void visit(LiteralTypeNode &node) override {}

  void visit(FunctionTypeNode &node) override {}

  void visit(BooleanLiteralNode &node) override {
    value = llvm::ConstantInt::get(llvm::IntegerType::get(*llvmContext, 1),
                                   node.value ? 1 : 0);
  }

  void visit(NumericLiteralNode &node) override {
    value = node.hasFloatingPoint
                ? llvm::ConstantFP::get(float64Type, node.value)
                : llvm::ConstantInt::get(int64Type, node.value);
  }

  void visit(CharLiteralNode &node) override {
    value =
        llvm::ConstantInt::get(llvm::Type::getInt8Ty(*llvmContext), node.value);
  }

  void visit(StringLiteralNode &node) override {
    value =
        builder->CreateGlobalStringPtr(llvm::StringRef(node.text), "global_s");
  }

  void visit(ArrowFunctionExpressionNode &node) override {
    std::string scopeName =
        "anonymous" + std::to_string(arrowFunctionExpressionIndex++);

    auto t = symbolTableVisitor.getType(&node);

    auto functionType = asFunctionType(*t, voidPointerType);

    symbolTableVisitor.enterScope(scopeName);

    auto *bb = builder->GetInsertBlock();

    std::vector<llvm::Type *> params{};
    std::vector<std::string> names{};

    names.push_back("env");
    for (const auto &parameterNode : node.parameters) {
      auto *parameter = dynamic_cast<ParameterNode *>(parameterNode.get());
      names.push_back(parameter->name);
    }

    auto function = llvm::Function::Create(
        functionType, llvm::Function::ExternalLinkage, "lambda", *llvmModule);

    unsigned index = 0;
    for (auto &arg : function->args()) {
      arg.setName(names[index++]);
    }

    auto mainBlock =
        llvm::BasicBlock::Create(*llvmContext, "main_block", function);
    builder->SetInsertPoint(mainBlock);

    auto copy = namedValues;
    namedValues.clear();
    for (auto &arg : function->args()) {
      llvm::AllocaInst *alloca =
          createEntryBlockAlloca(function, arg.getName(), arg.getType());
      builder->CreateStore(&arg, alloca);
      namedValues[std::string(arg.getName())] = alloca;
    }

    node.body->accept(*this);
    namedValues = copy;

    if (!mainBlock->getTerminator()) {
      builder->CreateRetVoid();
    }

    if (llvm::verifyFunction(*function, &llvm::outs())) {
      llvm::outs() << *function;
      throw std::runtime_error("Bad arrow function!");
    }

    value = function;
    builder->SetInsertPoint(bb);
    symbolTableVisitor.exitScope();

    std::vector<Symbol> capturedSymbols =
        symbolTableVisitor.currentScope->symbolTable.getAll(
            [](auto symbol) { return symbol.isSeen && symbol.isCaptured; });

    value = allocateClosure(function, capturedSymbols,
                            symbolTableVisitor.currentScope);
  }

  void visit(IdentifierNode &node) override {
    if (node.name == "null") {
      value =
          llvm::Constant::getNullValue(llvm::Type::getInt8PtrTy(*llvmContext));
      return;
    }

    auto moduleFunction = llvmModule->getFunction(node.name);
    if (moduleFunction) {
      std::vector<Symbol> symbols;
      value = allocateClosure(moduleFunction, symbols, nullptr);
      return;
    }

    value = localOrEnv(node.name, isWrite);
  };

  void visit(LetStatementNode &node) override {
    auto symbol = symbolTableVisitor.currentScope->lookup(node.name).value();

    auto type = buildLLVMType(*symbol.type);

    llvm::Function *TheFunction = builder->GetInsertBlock()->getParent();

    llvm::AllocaInst *alloca =
        createEntryBlockAlloca(TheFunction, node.name, type);
    namedValues[node.name] = alloca;

    if (!node.expression) {
      return;
    }
    node.expression->accept(*this);

    if (llvm::AllocaInst *rightAlloca = dyn_cast<llvm::AllocaInst>(value)) {
      namedValues[node.name] = rightAlloca;
    } else {
      namedValues[node.name] = alloca;
      builder->CreateStore(value, alloca);
    }

    symbolTableVisitor.currentScope->symbolTable.setSeen(symbol.name);
  };

  void visit(ForStatementNode &node) override {
    std::string scopeName = "for" + std::to_string(forScopeIndex++);
    symbolTableVisitor.enterScope(scopeName);

    llvm::Function *function = builder->GetInsertBlock()->getParent();

    llvm::BasicBlock *conditionBB =
        llvm::BasicBlock::Create(*llvmContext, "for_condition", function);
    llvm::BasicBlock *bodyBB =
        llvm::BasicBlock::Create(*llvmContext, "for_body", function);
    llvm::BasicBlock *mergeBB =
        llvm::BasicBlock::Create(*llvmContext, "for_continued");

    node.initializer->accept(*this);

    builder->CreateBr(conditionBB);
    builder->SetInsertPoint(conditionBB);
    node.condition->accept(*this);
    builder->CreateCondBr(value, bodyBB, mergeBB);

    builder->SetInsertPoint(bodyBB);
    node.body->accept(*this);
    node.incrementer->accept(*this);
    builder->CreateBr(conditionBB);

    function->insert(function->end(), mergeBB);
    builder->SetInsertPoint(mergeBB);

    symbolTableVisitor.exitScope();
  };

  void visit(ReturnStatementNode &node) override {
    if (node.expression) {
      node.expression->accept(*this);
      builder->CreateRet(value);
      return;
    }
    builder->CreateRetVoid();
  };

  void visit(ExpressionStatementNode &node) override {
    node.expression->accept(*this);
  };

  void visit(IfStatementNode &node) override {
    node.condition->accept(*this);
    auto conditionValue = value;

    llvm::Function *function = builder->GetInsertBlock()->getParent();

    llvm::BasicBlock *thenBB =
        llvm::BasicBlock::Create(*llvmContext, "then", function);
    llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(*llvmContext, "else");
    llvm::BasicBlock *mergeBB =
        llvm::BasicBlock::Create(*llvmContext, "if_continued");

    builder->CreateCondBr(conditionValue, thenBB, elseBB);

    builder->SetInsertPoint(thenBB);
    node.ifTrue->accept(*this);
    llvm::Value *thenValue = value;
    if (!thenValue)
      throw std::runtime_error("Error: unexpected error");

    thenBB = builder->GetInsertBlock();
    if (!thenBB->getTerminator()) {
      builder->CreateBr(mergeBB);
    }

    function->insert(function->end(), elseBB);
    builder->SetInsertPoint(elseBB);

    if (node.ifFalse) {
      node.ifFalse->accept(*this);
    }

    builder->CreateBr(mergeBB);
    elseBB = builder->GetInsertBlock();

    function->insert(function->end(), mergeBB);
    builder->SetInsertPoint(mergeBB);
  };

  void visit(ElementAccessExpressionNode &node) override {
    auto expressionType = symbolTableVisitor.getType(node.expression.get());
    ArrayType *arrayType = std::get_if<ArrayType>(&*expressionType);

    auto p = isWrite;

    isWrite = arrayType != nullptr;
    node.expression->accept(*this);
    isWrite = p;
    auto expressionValue = value;

    auto _isWrite = isWrite;
    isWrite = false;
    node.argumentExpression->accept(*this);
    auto argumentValue = value;
    isWrite = _isWrite;

    if (arrayType) {
      auto gep = builder->CreateInBoundsGEP(
          buildLLVMType(*arrayType), expressionValue,
          {llvm::ConstantInt::get(int64Type, 0), argumentValue});
      value = isWrite ? gep
                      : value = builder->CreateLoad(
                            buildLLVMType(*arrayType->type), gep);
      return;
    }

    PointerType *pointerType = std::get_if<PointerType>(&*expressionType);
    auto type = pointerType != nullptr ? buildLLVMType(*pointerType->type)
                                       : llvm::Type::getInt8Ty(*llvmContext);
    auto gep =
        builder->CreateInBoundsGEP(type, expressionValue, {argumentValue});
    value = isWrite ? gep : builder->CreateLoad(type, gep);
  }

  void visit(UnaryExpressionNode &node) override {
    node.expression->accept(*this);
    auto expressionValue = value;

    switch (node.op) {
    case Token::Kind::Asterisk: {
      auto expressionType = symbolTableVisitor.getType(node.expression.get());

      PointerType *pointerType = std::get_if<PointerType>(&*expressionType);
      if (!pointerType) {
        throw std::runtime_error("Expected pointer type");
      }

      value = isWrite ? builder->CreateLoad(buildLLVMType(*pointerType),
                                            expressionValue)
                      : builder->CreateLoad(buildLLVMType(*pointerType->type),
                                            expressionValue);
      break;
    }
    case Token::Kind::Minus: {
      value = builder->CreateNSWSub(
          llvm::Constant::getNullValue(expressionValue->getType()),
          expressionValue);
      break;
    }
    case Token::Kind::ExclamationMark: {
      value = builder->CreateNot(expressionValue);
      break;
    }
    default:
      throw std::runtime_error("Error: unknown unary operator: " +
                               std::string(kindToString(node.op)));
    }
  }

  void visit(BinaryExpressionNode &node) override {
    switch (node.op) {
    case Token::Kind::Equals: {
      node.rhs->accept(*this);
      llvm::Value *R = value;

      auto _isWrite = isWrite;
      isWrite = true;
      node.lhs->accept(*this);
      llvm::Value *L = value;
      isWrite = _isWrite;

      builder->CreateStore(R, value);
      value = L;
      break;
    }
    case Token::Kind::Dot: {
      auto _isWrite = isWrite;
      isWrite = true;
      node.lhs->accept(*this);
      isWrite = _isWrite;
      llvm::Value *L = value;

      IdentifierNode *rightIdentifierNode =
          dynamic_cast<IdentifierNode *>(node.rhs.get());
      if (!rightIdentifierNode) {
        throw std::runtime_error(
            "Error: right-hand side needs to be an identifier.");
      }

      auto lhsType = symbolTableVisitor.getType(node.lhs.get());

      StructType *structType = symbolTableVisitor.asStruct(lhsType.get());
      auto gep = builder->CreateStructGEP(
          buildLLVMType(*lhsType), L,
          findIndex(*structType, rightIdentifierNode->name));

      value = isWrite ? gep : builder->CreateLoad(int64Type, gep);

      break;
    }
    case Token::Kind::Asterisk: {
      node.lhs->accept(*this);
      llvm::Value *L = value;

      node.rhs->accept(*this);
      llvm::Value *R = value;

      if (L->getType()->isDoubleTy() && R->getType()->isDoubleTy()) {
        value = builder->CreateFMul(L, R);
        break;
      }
      value = builder->CreateMul(L, R);
      break;
    }
    case Token::Kind::Slash: {
      node.lhs->accept(*this);
      llvm::Value *L = value;

      node.rhs->accept(*this);
      llvm::Value *R = value;

      if (L->getType()->isDoubleTy() && R->getType()->isDoubleTy()) {
        value = builder->CreateFDiv(L, R);
        break;
      }
      value = builder->CreateSDiv(L, R);
      break;
    }
    case Token::Kind::Percent: {
      node.lhs->accept(*this);
      llvm::Value *L = value;

      node.rhs->accept(*this);
      llvm::Value *R = value;

      if (L->getType()->isDoubleTy() && R->getType()->isDoubleTy()) {
        value = builder->CreateFRem(L, R);
        break;
      }
      value = builder->CreateSRem(L, R);
      break;
    }
    case Token::Kind::Plus: {
      node.lhs->accept(*this);
      llvm::Value *L = value;

      node.rhs->accept(*this);
      llvm::Value *R = value;

      if (L->getType()->isDoubleTy() && R->getType()->isDoubleTy()) {
        value = builder->CreateFAdd(L, R);
        break;
      }
      value = builder->CreateAdd(L, R);
      break;
    }
    case Token::Kind::Minus: {
      node.lhs->accept(*this);
      llvm::Value *L = value;

      node.rhs->accept(*this);
      llvm::Value *R = value;

      if (L->getType()->isDoubleTy() && R->getType()->isDoubleTy()) {
        value = builder->CreateFSub(L, R);
        break;
      }
      value = builder->CreateSub(L, R);
      break;
    }
    case Token::Kind::DoubleEquals: {
      node.lhs->accept(*this);
      llvm::Value *L = value;

      node.rhs->accept(*this);
      llvm::Value *R = value;

      value = builder->CreateICmpEQ(L, R);
      break;
    }
    case Token::Kind::ExclamationMarkEquals: {
      node.lhs->accept(*this);
      llvm::Value *L = value;

      node.rhs->accept(*this);
      llvm::Value *R = value;

      value = builder->CreateICmpNE(L, R);
      break;
    }
    case Token::Kind::LessThan: {
      node.lhs->accept(*this);
      llvm::Value *L = value;

      node.rhs->accept(*this);
      llvm::Value *R = value;

      value = builder->CreateICmpSLT(L, R);
      break;
    }
    case Token::Kind::GreaterThan: {
      node.lhs->accept(*this);
      llvm::Value *L = value;

      node.rhs->accept(*this);
      llvm::Value *R = value;

      value = builder->CreateICmpSGT(L, R);
      break;
    }
    case Token::Kind::LessThanEquals: {
      node.lhs->accept(*this);
      llvm::Value *L = value;

      node.rhs->accept(*this);
      llvm::Value *R = value;

      value = builder->CreateICmpSLE(L, R);
      break;
    }
    case Token::Kind::GreaterThanEquals: {
      node.lhs->accept(*this);
      llvm::Value *L = value;

      node.rhs->accept(*this);
      llvm::Value *R = value;

      value = builder->CreateICmpSGE(L, R);
      break;
    }
    case Token::Kind::AmpersandAmpersand: {
      node.lhs->accept(*this);
      llvm::Value *L = value;

      node.rhs->accept(*this);
      llvm::Value *R = value;

      value = builder->CreateLogicalAnd(L, R);
      break;
    }
    case Token::Kind::BarBar: {
      node.lhs->accept(*this);
      llvm::Value *L = value;

      node.rhs->accept(*this);
      llvm::Value *R = value;

      value = builder->CreateLogicalOr(L, R);
      break;
    }

    default:
      throw std::runtime_error("Error: unknown operator: " +
                               std::string(kindToString(node.op)));
    };
  }

  llvm::Function *getFunction(const std::string &name,
                              std::vector<llvm::Value *> arguments) {
    return llvmModule->getFunction(name);
  }

  void visit(CallExpressionNode &node) override {
    if (node.callee == "sizeof") {
      auto type = symbolTableVisitor.getType(node.arguments.front().get());
      value = llvm::ConstantInt::get(
          int64Type,
          llvmModule->getDataLayout().getTypeAllocSize(buildLLVMType(*type)));
      return;
    }

    std::vector<llvm::Value *> arguments;
    for (unsigned i = 0, e = node.arguments.size(); i != e; ++i) {
      node.arguments[i]->accept(*this);
      arguments.push_back(value);
    }

    auto function = getFunction(node.callee, arguments);
    if (function) {
      arguments.insert(arguments.begin(),
                       llvm::Constant::getNullValue(voidPointerType));
      value = builder->CreateCall(function, arguments);
      return;
    }

    auto symbol = symbolTableVisitor.currentScope->lookup(node.callee).value();
    auto type = *symbol.type;

    auto callType = asFunctionType(type, voidPointerType);
    auto closureType = getClosureType();

    llvm::Value *kValue = localOrEnv(node.callee, true);

    // llvm::outs() << "callee = " << node.callee << ", callType = "
    // << *callType
    //              << ", closureType = " << *closureType
    //              << ", value = " << *kValue << "\n";

    auto closure =
        builder->CreateLoad(closureType->getPointerTo(), kValue, "closure");

    auto env = builder->CreateLoad(
        voidPointerType, builder->CreateStructGEP(closureType, closure, 1),
        "env");

    auto fn = builder->CreateLoad(
        callType->getPointerTo(),
        builder->CreateStructGEP(closureType, closure, 0), "closure_call_fn");

    arguments.insert(arguments.begin(), env);
    value = builder->CreateCall(callType, fn, arguments);
  };

  void visit(BlockNode &node) override {
    for (const auto &statement : node.statements) {
      statement->accept(*this);
    }
  };

  void visit(ParameterNode &node) override{};

  void visit(PropertySignatureNode &node) override {
    // TODO
  }

  void visit(PropertyAssignmentNode &node) override {
    // TODO
  }

  void visit(ObjectLiteralNode &node) override {
    auto type = symbolTableVisitor.getType(&node);
    auto llvmType = buildLLVMType(*type);

    llvm::Function *function = builder->GetInsertBlock()->getParent();
    llvm::AllocaInst *objectLiteralAlloca =
        createEntryBlockAlloca(function, "object_literal", llvmType);

    llvm::StructType *structType = dyn_cast<llvm::StructType>(llvmType);
    if (!structType) {
      throw std::runtime_error("Expected struct type");
    }

    for (auto index = 0; const auto &propertyNode : node.properties) {
      auto *property =
          dynamic_cast<PropertyAssignmentNode *>(propertyNode.get());
      assert(property);
      property->initializer->accept(*this);
      auto gep = builder->CreateStructGEP(structType, objectLiteralAlloca,
                                          index++, property->name);
      if (llvm::AllocaInst *rightAlloca = dyn_cast<llvm::AllocaInst>(value)) {
        builder->CreateMemCpy(
            gep, llvm::MaybeAlign(), rightAlloca, llvm::MaybeAlign(),
            llvm::ConstantInt::get(int64Type,
                                   llvmModule->getDataLayout().getTypeAllocSize(
                                       rightAlloca->getAllocatedType())));
      } else {
        builder->CreateStore(value, gep);
      }
    }
    value = builder->CreateLoad(llvmType, objectLiteralAlloca);
  }

  void visit(StructTypeNode &node) override {
    // TODO
  }

  void visit(InterfaceDeclarationNode &node) override {
    // TODO
  }

  void visit(FunctionDeclarationNode &node) override {
    auto symbol = symbolTableVisitor.currentScope->lookup(node.name, false);
    auto functionType = asFunctionType(*symbol->type, voidPointerType);

    std::vector<llvm::Type *> params{};
    std::vector<std::string> names{};

    names.push_back("env");
    for (const auto &parameterNode : node.parameters) {
      auto *parameter = dynamic_cast<ParameterNode *>(parameterNode.get());
      names.push_back(parameter->name);
    }

    auto function =
        llvm::Function::Create(functionType, llvm::Function::ExternalLinkage,
                               node.name, llvmModule.get());

    unsigned index = 0;
    for (auto &arg : function->args()) {
      arg.setName(names[index++]);
    }

    if (!node.body) {
      return;
    }

    symbolTableVisitor.enterScope(node.name);

    auto mainBlock =
        llvm::BasicBlock::Create(*llvmContext, "main_block", function);
    builder->SetInsertPoint(mainBlock);

    namedValues.clear();
    for (auto &arg : function->args()) {
      if (arg.getName() != "env") {
        symbolTableVisitor.currentScope->symbolTable.setSeen(
            std::string(arg.getName()));
      }
      llvm::AllocaInst *alloca =
          createEntryBlockAlloca(function, arg.getName(), arg.getType());
      builder->CreateStore(&arg, alloca);
      namedValues[std::string(arg.getName())] = alloca;
    }

    if (node.name == "main") {
      builder->CreateCall(llvmModule->getFunction("beforeStart"), {});
    }

    node.body->accept(*this);

    mainBlock = builder->GetInsertBlock();
    if (!mainBlock->getTerminator()) {
      builder->CreateRetVoid();
    }

    if (node.name == "main") {
      mainBlock = builder->GetInsertBlock();
      builder->SetInsertPoint(mainBlock->getTerminator());
      builder->CreateCall(llvmModule->getFunction("beforeExit"), {});
    }

    if (llvm::verifyFunction(*function, &llvm::outs())) {
      llvm::outs() << "\n" << *llvmModule << "\n";
      throw std::runtime_error("Bad function!");
    }

    symbolTableVisitor.exitScope();
  };

  void visit(SourceFileNode &node) override {
    for (const auto &function : node.functions) {
      function->accept(*this);
    }
  };

  int compile(const std::string fileName) {
    create_beforeStart_call();
    create_beforeExit_call();
    createAllocateFunction();

    sourceFile.accept(*this);

    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    auto targetTriple = llvm::sys::getDefaultTargetTriple();
    llvmModule->setTargetTriple(targetTriple);

    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(targetTriple, error);

    llvm::TargetOptions options;
    auto rm = std::optional<llvm::Reloc::Model>();
    auto targetMachine =
        target->createTargetMachine(targetTriple, "generic", "", options, rm);

    llvmModule->setDataLayout(targetMachine->createDataLayout());

    if (llvm::verifyModule(*llvmModule, &llvm::outs())) {
      debug << "Bad module!" << std::endl;
      return 1;
    }

    // llvm::outs() << "\n---\n" << *llvmModule << "\n---\n";

    std::error_code errorCode;
    llvm::raw_fd_ostream destination(fileName, errorCode,
                                     llvm::sys::fs::OF_None);

    if (errorCode) {
      llvm::errs() << "Could not open file: " << errorCode.message();
      return 1;
    }

    llvm::legacy::PassManager pass;
    auto fileType = llvm::CodeGenFileType::CGFT_ObjectFile;
    if (targetMachine->addPassesToEmitFile(pass, destination, nullptr,
                                           fileType)) {
      llvm::errs() << "TheTargetMachine can't emit a file of this type";
      return 1;
    }

    pass.run(*llvmModule);
    destination.flush();

    return 0;
  }
};