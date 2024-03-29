#pragma once

#include <algorithm> // find_if
#include <iomanip>
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "Debug.hpp"
#include "Node.hpp"
#include "Printer.hpp"
#include "Type.hpp"

struct Symbol {
  bool isInternal = false;
  bool isCaptured = false;
  bool isSeen = false;

  // TODO: move to FunctionType
  bool isGeneric = false;
  std::vector<std::shared_ptr<Type>> genericTypeInstances;

  std::string name;
  std::shared_ptr<Type> type;

  void print() const {
    std::cout << "Symbol { name = " << name
              << ", type = " << typeToString(*type) << " }" << std::endl;
  }
};

class SymbolTable {
  std::map<std::string, Symbol> data;

public:
  void removePublicSymbols() {
    std::erase_if(data, [](const auto &item) {
      auto const &[key, value] = item;
      return !value.isInternal;
    });
  }

  void setCaptured(std::string symbolName, bool isCaptured) {
    debug << "Captured " << symbolName << std::endl;
    assert(data.contains(symbolName));
    data[symbolName].isCaptured = isCaptured;
  }

  void setSeen(std::string symbolName) {
    assert(data.contains(symbolName));
    data[symbolName].isSeen = true;
  }

  bool allInternal() const {
    auto all = getAll();
    if (all.empty()) {
      return false;
    }
    return std::all_of(all.cbegin(), all.cend(),
                       [](const Symbol &symbol) { return symbol.isInternal; });
  }

  bool containsSymbol(const std::string &name) const {
    return data.contains(name);
  }

  void removeSymbol(const std::string &name) { data.erase(name); }

  void addSymbol(const Symbol &symbol) { data[symbol.name] = symbol; }

  Symbol &getRef(const std::string &name) { return data[name]; }

  std::optional<Symbol> get(const std::string &name) const {
    auto it = data.find(name);
    if (it != data.end()) {
      return it->second;
    }
    return {};
  }

  int getIndex(const std::string &name) const {
    auto it = data.find(name);
    if (it == data.end()) {
      return -1;
    }
    return std::distance(data.begin(), it);
  }

  std::vector<Symbol> getAll(std::function<bool(const Symbol &)> filter) const {
    std::vector<Symbol> result;
    for (auto &elem : data) {
      if (filter(elem.second)) {
        result.push_back(elem.second);
      }
    }
    return result;
  }

  std::vector<Symbol> getAll() const {
    std::vector<Symbol> result;
    result.reserve(data.size());
    for (const auto &elem : data) {
      result.push_back(elem.second);
    }
    return result;
  }

  void print(int level) const {
    for (const auto &elem : data) {
      if (elem.second.isInternal) {
        continue;
      }
      std::cout << std::string(level * 2, ' ');
      std::cout << elem.second.name << ": " << typeToString(*elem.second.type)
                << std::endl;
    }
  }
};

class Scope {
public:
  std::string name;
  Scope *parent;
  std::vector<std::shared_ptr<Scope>> scopes;
  SymbolTable symbolTable;

  Scope(std::string name, Scope *parent) : name(name), parent(parent){};

  bool hasSymbols() const { return !symbolTable.getAll().empty(); }

  Scope *createChildScope(std::string name) {
    auto scope = std::make_shared<Scope>(name, this);
    scopes.push_back(scope);
    return scope.get();
  }

  Symbol get(const std::string &name, bool traverseParent = true) const {
    auto symbol = lookup(name, traverseParent);
    if (!symbol) {
      throw std::runtime_error("Could not find symbol '" + name + "'");
    }
    return *symbol;
  }

  std::optional<std::pair<Symbol, Scope *>>
  lookupWithScope(const std::string &name, bool traverseParent = true) const {
    if (auto symbol = symbolTable.get(name)) {
      return std::make_pair(*symbol, (Scope *)this);
    }
    if (!traverseParent) {
      return {};
    }
    if (parent) {
      return parent->lookupWithScope(name);
    }
    return {};
  }

  Symbol &lookupRef(const std::string &name, bool traverseParent = true) {
    if (symbolTable.containsSymbol(name)) {
      return symbolTable.getRef(name);
    }
    if (!traverseParent) {
      throw std::runtime_error("Can't find symbol " + name);
    }
    if (parent) {
      return parent->lookupRef(name);
    }
    throw std::runtime_error("Can't find symbol " + name);
  }

  std::optional<Symbol> lookup(const std::string &name,
                               bool traverseParent = true) const {
    if (auto symbol = symbolTable.get(name)) {
      return *symbol;
    }
    if (!traverseParent) {
      return {};
    }
    if (parent) {
      return parent->lookup(name);
    }
    return {};
  }

  void removePublicSymbols() {
    symbolTable.removePublicSymbols();
    for (const auto &scope : scopes) {
      (*scope).removePublicSymbols();
    }
    scopes.erase(
        std::remove_if(scopes.begin(), scopes.end(),
                       [](const auto scope) { return !scope->hasSymbols(); }),
        scopes.end());
  }

  void print(int level = 0) const {
    if (symbolTable.allInternal()) {
      return;
    }
    std::cout << std::string(level * 2, ' ') << "Table '" << name << "'"
              << std::endl;
    symbolTable.print(level);
    std::cout << std::endl;
    for (const auto &scope : scopes) {
      (*scope).print(level + 1);
    }
  }
};

class SymbolTableVisitor : public Visitor {
private:
  enum class SymbolTableVisitorMode {
    Public,
    Internal,
  };
  SymbolTableVisitorMode mode;

  std::map<Node *, std::shared_ptr<Type>> nodeToType;

  void setType(const Node &node, const std::shared_ptr<Type> &type) {
    nodeToType[(Node *)&node] = type;
  }

public:
  Scope globalScope;
  Scope *currentScope;

  size_t arrowFunctionExpressionIndex = 0;
  size_t forScopeIndex = 0;

  void setInternalMode(bool flag) {
    mode = flag ? SymbolTableVisitorMode::Internal
                : SymbolTableVisitorMode::Public;
  }

  void removePublicSymbols() {
    // reset
    arrowFunctionExpressionIndex = 0;
    forScopeIndex = 0;
    globalScope.removePublicSymbols();
  }

  void enterScope(std::string name) {
    auto it =
        std::find_if(currentScope->scopes.begin(), currentScope->scopes.end(),
                     [&name](auto x) { return x->name == name; });
    if (it == currentScope->scopes.end()) {
      throw std::runtime_error("Could not find scope for '" + name + "'");
    }
    currentScope = (*it).get();
  }

  void createScopeAndEnter(std::string name) {
    currentScope = currentScope->createChildScope(name);
  }

  void exitScope() { currentScope = currentScope->parent; }

  std::shared_ptr<Type> getType(Node *node) const {
    if (!nodeToType.contains(node)) {
      Printer p{*node};
      p.print();
    }

    return nodeToType.at(node);
  }

public:
  SymbolTableVisitor()
      : globalScope("global", nullptr), currentScope(&globalScope) {}

  void visit(TypeReferenceNode &node) override {
    auto *identifier = dynamic_cast<IdentifierNode *>(node.typeName.get());
    assert(identifier);

    if (identifier->name == "Pointer") {
      auto type = typeNodeToType(node.typeParameters.front().get());
      setType(node, std::make_shared<Type>(PointerType(type)));
      return;
    }
    if (identifier->name == "Array") {
      setType(node, typeNodeToType(&node));
      return;
    }
    auto symbol = currentScope->get(identifier->name);
    setType(node, symbol.type);
  }

  void visit(FunctionTypeNode &node) override {}

  void visit(BooleanLiteralNode &node) override {
    setType(node, std::make_shared<Type>(PrimitiveType::booleanType));
  }

  void visit(LiteralTypeNode &node) override {
    setType(node, std::make_shared<Type>(PrimitiveType::i64Type));
  }

  void visit(NumericLiteralNode &node) override {
    setType(node, std::make_shared<Type>(node.hasFloatingPoint
                                             ? PrimitiveType::f64Type
                                             : PrimitiveType::i64Type));
  }

  void visit(CharLiteralNode &node) override {
    setType(node, std::make_shared<Type>(PrimitiveType::charType));
  }

  void visit(StringLiteralNode &node) override {
    setType(node, std::make_shared<Type>(PrimitiveType::stringType));
  }

  void visit(IdentifierNode &node) override {
    if (node.name == "i64") {
      setType(node, std::make_shared<Type>(PrimitiveType::i64Type));
      return;
    }

    if (node.name == "string") {
      setType(node, std::make_shared<Type>(PrimitiveType::stringType));
      return;
    }

    if (node.name == "null") {
      setType(node, std::make_shared<Type>(PointerType(
                        std::make_shared<Type>(PrimitiveType::voidType))));
      return;
    }

    auto symbol = currentScope->get(node.name);
    if (TypeReference *typeReference =
            std::get_if<TypeReference>(&*symbol.type)) {
      setType(node, currentScope->lookup(typeReference->name)->type);
      return;
    }
    setType(node, symbol.type);
  };

  void visit(ReturnStatementNode &node) override {
    if (node.expression) {
      node.expression->accept(*this);
    }
  };

  void visit(ExpressionStatementNode &node) override {
    node.expression->accept(*this);
  };

  void visit(IfStatementNode &node) override {
    node.condition->accept(*this);
    node.ifTrue->accept(*this);
    if (node.ifFalse) {
      node.ifFalse->accept(*this);
    }
  };

  void visit(ElementAccessExpressionNode &node) override {
    node.expression->accept(*this);
    node.argumentExpression->accept(*this);

    ArrayType *arrayType =
        std::get_if<ArrayType>(&*getType(node.expression.get()));
    if (arrayType) {
      setType(node, arrayType->type);
      return;
    }

    PointerType *pointerType =
        std::get_if<PointerType>(&*getType(node.expression.get()));
    if (pointerType) {
      setType(node, pointerType->type);
      return;
    }

    setType(node, std::make_shared<Type>(PrimitiveType::charType));
  };

  void visit(UnaryExpressionNode &node) override {
    node.expression->accept(*this);

    if (node.op == Token::Kind::Asterisk) {
      PointerType *pointerType =
          std::get_if<PointerType>(&*getType(node.expression.get()));
      if (!pointerType) {
        throw std::runtime_error("Expected pointer type");
      }
      setType(node, pointerType->type);
      return;
    }

    setType(node, getType(node.expression.get()));
  }

  StructType *asStruct(Type *type) {
    TypeReference *typeReference = std::get_if<TypeReference>(&*type);
    if (typeReference) {
      auto symbol = currentScope->get(typeReference->name);
      return asStruct(symbol.type.get());
    }

    StructType *structType = std::get_if<StructType>(&*type);
    if (structType) {
      return structType;
    }

    throw std::runtime_error("Could not cast type to struct");
  }

  void visit(BinaryExpressionNode &node) override {
    node.lhs->accept(*this);

    if (node.op != Token::Kind::Dot) {
      node.rhs->accept(*this);
      setType(node, getType(node.lhs.get()));
      return;
    }

    auto *rhsIdentifier = dynamic_cast<IdentifierNode *>(node.rhs.get());
    if (!rhsIdentifier) {
      throw std::runtime_error("Expected identifier as rhs");
    }

    auto lhsType = getType(node.lhs.get());
    StructType *structType = asStruct(lhsType.get());
    for (const auto &property : structType->properties) {
      if (property.first == rhsIdentifier->name) {
        setType(node, property.second);
        return;
      }
    }
  };

  void visit(CallExpressionNode &node) override {
    for (const auto &argument : node.arguments) {
      argument->accept(*this);
    }

    if (node.callee == "sizeof") {
      setType(node, std::make_shared<Type>(PrimitiveType::i64Type));
    } else if (node.callee == "print") {
      // TODO
    } else {
      auto &symbol = currentScope->lookupRef(node.callee);
      if (FunctionType *functionType =
              std::get_if<FunctionType>(symbol.type.get())) {
        if (symbol.isGeneric) {
          std::vector<std::shared_ptr<Type>> argumentTypes(
              node.arguments.size());

          std::transform(node.arguments.cbegin(), node.arguments.cend(),
                         argumentTypes.begin(), [&](auto &argument) {
                           return getType(argument.get());
                         });

          auto typeArgument = argumentTypes.front();
          symbol.genericTypeInstances.push_back(typeArgument);
          setType(node, functionType->solve(typeArgument)->returnType);
          return;
        }

        setType(node, functionType->returnType);
      }
    }
  };

  Symbol createSymbol(const std::string &name,
                      const std::shared_ptr<Type> &type) const {
    return Symbol{.name = name,
                  .type = type,
                  .isCaptured = false,
                  .isInternal = mode == SymbolTableVisitorMode::Internal};
  }

  void visit(LetStatementNode &node) override {
    if (node.type) {
      node.type->accept(*this);
    }
    if (node.expression) {
      node.expression->accept(*this);
    }
    auto type =
        node.type ? getType(node.type.get()) : getType(node.expression.get());
    Symbol symbol = createSymbol(node.name, type);

    currentScope->symbolTable.addSymbol(symbol);
  }

  void visit(ForStatementNode &node) override {
    std::string scopeName = "for" + std::to_string(forScopeIndex++);
    createScopeAndEnter(scopeName);

    node.initializer->accept(*this);
    node.condition->accept(*this);
    node.incrementer->accept(*this);
    node.body->accept(*this);

    exitScope();
  }

  void visit(BlockNode &node) override {
    for (const auto &statement : node.statements) {
      statement->accept(*this);
    }
  };

  void visit(PropertySignatureNode &node) override{
      // TODO
  };

  void visit(PropertyAssignmentNode &node) override{
      // TODO
  };

  void visit(ObjectLiteralNode &node) override {
    std::vector<NamedType> properties;
    for (const auto &propertyNode : node.properties) {
      auto *property =
          dynamic_cast<PropertyAssignmentNode *>(propertyNode.get());
      assert(property);
      property->initializer->accept(*this);
      properties.push_back(
          std::make_pair(property->name, getType(property->initializer.get())));
    }
    setType(node, std::make_shared<Type>(StructType{properties}));
  };

  void visit(ParameterNode &node) override {
    Symbol symbol = createSymbol(node.name, typeNodeToType(node.type.get()));
    currentScope->symbolTable.addSymbol(symbol);
  };

  void visit(ArrowFunctionExpressionNode &node) override {
    std::vector<std::shared_ptr<Type>> parameters;
    for (auto &parameterNode : node.parameters) {
      auto *parameter = dynamic_cast<ParameterNode *>(parameterNode.get());
      parameters.push_back(typeNodeToType(parameter->type.get()));
    }
    auto functionType = std::make_shared<Type>(FunctionType{
        typeNodeToType(node.returnType.get()), std::move(parameters)});
    setType(node, functionType);

    std::string scopeName =
        "anonymous" + std::to_string(arrowFunctionExpressionIndex++);

    createScopeAndEnter(scopeName);
    for (const auto &parameter : node.parameters) {
      parameter->accept(*this);
    }
    node.body->accept(*this);
    exitScope();
  };

  void visit(StructTypeNode &node) override {
    // TODO
  }

  void visit(InterfaceDeclarationNode &node) override {
    Symbol symbol = createSymbol(node.name, typeNodeToType(&node));
    currentScope->symbolTable.addSymbol(symbol);

    // std::vector<std::shared_ptr<Type>> parameters;
    // auto functionType = std::make_shared<Type>(
    //     FunctionType{std::make_shared<Type>(PointerType(symbol.type)),
    //                  std::move(parameters)});

    // Symbol xsymbol = createSymbol("newList", functionType);
    // currentScope->symbolTable.addSymbol(xsymbol);
  }

  void visit(FunctionDeclarationNode &node) override {
    std::vector<std::shared_ptr<Type>> parameters;
    for (auto &parameterNode : node.parameters) {
      auto *parameter = dynamic_cast<ParameterNode *>(parameterNode.get());
      parameters.push_back(typeNodeToType(parameter->type.get()));
    }
    auto functionType = std::make_shared<Type>(FunctionType{
        typeNodeToType(node.returnType.get()), std::move(parameters)});
    Symbol symbol = createSymbol(node.name, std::move(functionType));
    symbol.isGeneric = node.typeParameters.size() > 0;
    currentScope->symbolTable.addSymbol(symbol);

    createScopeAndEnter(node.name);
    for (auto &parameter : node.parameters) {
      parameter->accept(*this);
    }
    if (node.body) {
      node.body->accept(*this);
    }
    exitScope();
  }

  void visit(SourceFileNode &node) override {
    for (const auto &interface : node.interfaces) {
      interface->accept(*this);
    }
    for (const auto &function : node.functions) {
      function->accept(*this);
    }
  };

  void createSymbolsFromSourceFile(SourceFileNode &node,
                                   bool markSymbolsAsInternal = false) {
    node.accept(*this);
  }

  void print() { globalScope.print(0); }
};