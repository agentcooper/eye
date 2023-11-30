#pragma once

#include "Debug.hpp"
#include "LLVM.hpp"
#include "Node.hpp"
#include "SymbolTable.hpp"

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
    case PrimitiveType::stringType:
      return llvm::Type::getInt8PtrTy(context);
    case PrimitiveType::voidType:
      return llvm::Type::getVoidTy(context);
    case PrimitiveType::unknownType:
      throw std::runtime_error("Can't get LLVM type for unknown type");
    }
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
                               const std::vector<Symbol> closureSymbols,
                               Scope *scope) {
    debug << "Allocating closure" << std::endl;

    auto envType = getEnvTypeForSymbols(closureSymbols);
    auto closureType = getClosureType();

    llvm::AllocaInst *envAlloca;

    if (!closureSymbols.empty()) {
      int envSize = llvmModule->getDataLayout().getTypeAllocSize(envType);
      envAlloca = scopeEnv.contains(scope)
                      ? scopeEnv[scope]
                      : createAllocateCall(envType, "env", envSize);
      scopeEnv[scope] = envAlloca;

      auto env = builder->CreateLoad(envType->getPointerTo(), envAlloca, "env");

      for (size_t idx = 0; const auto &symbol : closureSymbols) {
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

    if (!closureSymbols.empty()) {
      builder->CreateStore(
          builder->CreateLoad(envType->getPointerTo(), envAlloca, "zzz"),
          builder->CreateStructGEP(closureType, closure, 1));
    }

    return closure;
  }

  llvm::Type *getEnvTypeForSymbols(const std::vector<Symbol> &symbols) const {
    std::vector<llvm::Type *> types;
    std::transform(
        symbols.cbegin(), symbols.cend(), std::back_inserter(types),
        [&](const Symbol &symbol) { return buildLLVMType(*symbol.type); });
    return llvm::StructType::get(*llvmContext, types, false);
  }

  llvm::Type *getEnvTypeFromScope(const Scope *scope) const {
    auto symbols = scope->symbolTable.getAll();
    return getEnvTypeForSymbols(symbols);
  }

  llvm::Value *localOrEnv(const std::string &name, bool isRef) const {
    bool isLocal = true;
    auto symbol = symbolTableVisitor.currentScope->lookup(name, false);
    if (!symbol.has_value()) {
      isLocal = false;
      symbol = symbolTableVisitor.currentScope->lookup(name);
    }
    if (!symbol) {
      throw std::runtime_error("Could not find the symbol '" + name + "'");
    }

    auto llvmType = buildLLVMType(*symbol->type);

    if (isLocal) {
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
    auto parentSymbols = parentScope->symbolTable.getAll();

    auto index = parentScope->symbolTable.getIndex(name);
    debug << "Index of " << name << " is " << index << std::endl;

    auto envType = getEnvTypeFromScope(parentScope);
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
    auto call = builder->CreateCall(llvmModule->getFunction("allocate"),
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
                           "allocate", llvmModule.get());
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

  void visit(FunctionTypeNode &node) override {}

  void visit(NumericLiteralNode &node) override {
    value = node.hasFloatingPoint
                ? llvm::ConstantFP::get(float64Type, node.value)
                : llvm::ConstantInt::get(int64Type, node.value);
  }

  void visit(StringLiteralNode &node) override {
    value = builder->CreateGlobalStringPtr(llvm::StringRef(node.text));
  }

  void visit(ArrowFunctionExpressionNode &node) override {
    std::string scopeName =
        "anonymous" + std::to_string(arrowFunctionExpressionIndex++);

    auto t = symbolTableVisitor.getType(&node);

    auto functionType = asFunctionType(*t, voidPointerType);

    std::vector<Symbol> parentSymbols =
        symbolTableVisitor.currentScope->symbolTable.getAll();

    symbolTableVisitor.enterScope(scopeName);

    auto *bb = builder->GetInsertBlock();

    std::vector<llvm::Type *> params{};
    std::vector<std::string> names{};

    names.push_back("env");
    for (const auto &parameter : node.parameters) {
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
    value = allocateClosure(function, parentSymbols,
                            symbolTableVisitor.currentScope);
  }

  void visit(IdentifierNode &node) override {
    auto moduleFunction = llvmModule->getFunction(node.name);
    if (moduleFunction) {
      value = allocateClosure(moduleFunction, {}, nullptr);
      return;
    }

    value = localOrEnv(node.name, isWrite);
  };

  void visit(LetStatementNode &node) override {
    auto symbol = symbolTableVisitor.currentScope->lookup(node.name).value();
    auto type = buildLLVMType(*symbol.type);

    llvm::Function *TheFunction = builder->GetInsertBlock()->getParent();
    node.expression->accept(*this);

    llvm::AllocaInst *alloca =
        createEntryBlockAlloca(TheFunction, node.name, type);

    if (llvm::AllocaInst *rightAlloca = dyn_cast<llvm::AllocaInst>(value)) {
      namedValues[node.name] = rightAlloca;
    } else {
      namedValues[node.name] = alloca;
      builder->CreateStore(value, alloca);
    }
  };

  void visit(ReturnStatementNode &node) override {
    node.expression->accept(*this);
    builder->CreateRet(value);
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

    if (!thenBB->getTerminator()) {
      builder->CreateBr(mergeBB);
    }
    thenBB = builder->GetInsertBlock();

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

  void visit(BinaryExpressionNode &node) override {
    node.lhs->accept(*this);
    llvm::Value *L = value;

    switch (node.op) {
    case Token::Kind::Equals: {
      node.rhs->accept(*this);
      llvm::Value *R = value;

      isWrite = true;
      node.lhs->accept(*this);
      isWrite = false;

      builder->CreateStore(R, value);
      value = L;
      break;
    }
    case Token::Kind::Dot: {
      auto prevIsWrite = isWrite;

      isWrite = true;
      node.lhs->accept(*this);
      isWrite = prevIsWrite;
      llvm::Value *L = value;

      IdentifierNode *rightIdentifierNode =
          dynamic_cast<IdentifierNode *>(node.rhs.get());
      if (!rightIdentifierNode) {
        throw std::runtime_error(
            "Error: right-hand side needs to be an identifier.");
      }

      auto lhsType = symbolTableVisitor.getType(node.lhs.get());

      StructType *structType = std::get_if<StructType>(&*lhsType);
      if (!structType) {
        throw std::runtime_error("1 Expected struct type, but got " +
                                 typeToString(*lhsType));
      }

      auto gep = builder->CreateStructGEP(
          buildLLVMType(*lhsType), L,
          findIndex(*structType, rightIdentifierNode->name));

      value = isWrite ? gep : builder->CreateLoad(int64Type, gep);

      break;
    }
    case Token::Kind::Plus: {
      node.rhs->accept(*this);
      llvm::Value *R = value;

      if (L->getType()->isDoubleTy() && R->getType()->isDoubleTy()) {
        value = builder->CreateFAdd(L, R);
        break;
      }
      value = builder->CreateAdd(L, R, L->getName() + "_plus_" + R->getName());
      break;
    }
    case Token::Kind::Minus: {
      node.rhs->accept(*this);
      llvm::Value *R = value;

      if (L->getType()->isDoubleTy() && R->getType()->isDoubleTy()) {
        value = builder->CreateFSub(L, R);
        break;
      }
      value = builder->CreateSub(L, R, L->getName() + "_minus_" + R->getName());
      break;
    }
    case Token::Kind::DoubleEquals: {
      node.rhs->accept(*this);
      llvm::Value *R = value;

      value =
          builder->CreateICmpEQ(L, R, L->getName() + "_equals_" + R->getName());
      break;
    }
    default:
      throw std::runtime_error("Error: unknown operator: " +
                               std::string(kindToString(node.op)));
    };
  }

  // @TODO: this make it impossible to pass `print` function as an argument
  llvm::Function *getFunction(const std::string &name,
                              std::vector<llvm::Value *> arguments) {
    if (name == "print") {
      if (arguments[0]->getType()->isDoubleTy()) {
        return llvmModule->getFunction("printF64");
      } else if (arguments[0]->getType()->isPointerTy()) {
        return llvmModule->getFunction("printString");
      } else {
        return llvmModule->getFunction("printI64");
      }
    }
    return llvmModule->getFunction(name);
  }

  void visit(CallExpressionNode &node) override {
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

    for (auto index = 0; const auto &property : node.properties) {
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
    for (const auto &parameter : node.parameters) {
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
      llvm::AllocaInst *alloca =
          createEntryBlockAlloca(function, arg.getName(), arg.getType());
      builder->CreateStore(&arg, alloca);
      namedValues[std::string(arg.getName())] = alloca;
    }

    if (node.name == "main") {
      builder->CreateCall(llvmModule->getFunction("beforeStart"), {});
    }

    node.body->accept(*this);

    if (!mainBlock->getTerminator()) {
      builder->CreateRetVoid();
    }

    if (node.name == "main") {
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