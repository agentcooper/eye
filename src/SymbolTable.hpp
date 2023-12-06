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
  std::string name;
  std::shared_ptr<Type> type;

  void print() const {
    debug << "Symbol { name = " << name << ", type = " << typeToString(*type)
          << " }" << std::endl;
  }
};

class SymbolTable {
  std::map<std::string, Symbol> data;

public:
  void addSymbol(const Symbol &symbol) { data[symbol.name] = symbol; }

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

  std::vector<Symbol> getAll() const {
    std::vector<Symbol> result;
    for (const auto &elem : data) {
      result.push_back(elem.second);
    }
    return result;
  }

  void print(int level) const {
    for (const auto &elem : data) {
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

  void print(int level = 0) const {
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
  std::map<Node *, std::shared_ptr<Type>> nodeToType;

  void setType(const Node &node, const std::shared_ptr<Type> &type) {
    nodeToType[(Node *)&node] = type;
  }

public:
  Scope globalScope;
  Scope *currentScope;

  size_t arrowFunctionExpressionIndex = 0;
  size_t forScopeIndex = 0;

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
    auto symbol = currentScope->get(node.typeName->name);
    setType(node, symbol.type);
  }

  void visit(FunctionTypeNode &node) override {}

  void visit(NumericLiteralNode &node) override {
    setType(node, std::make_shared<Type>(node.hasFloatingPoint
                                             ? PrimitiveType::f64Type
                                             : PrimitiveType::i64Type));
  }

  void visit(StringLiteralNode &node) override {
    setType(node, std::make_shared<Type>(PrimitiveType::stringType));
  }

  void visit(IdentifierNode &node) override {
    auto symbol = currentScope->get(node.name);
    if (TypeReference *typeReference =
            std::get_if<TypeReference>(&*symbol.type)) {
      setType(node, currentScope->lookup(typeReference->name)->type);
      return;
    }
    setType(node, symbol.type);
  };

  void visit(ReturnStatementNode &node) override {
    node.expression->accept(*this);
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
    StructType *structType = std::get_if<StructType>(&*lhsType);
    if (!structType) {
      throw std::runtime_error("2 Expected struct type, but got " +
                               typeToString(*lhsType));
    }
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

    if (auto symbol = currentScope->lookup(node.callee)) {
      if (FunctionType *functionType =
              std::get_if<FunctionType>(&*symbol->type)) {
        setType(node, functionType->returnType);
      }
    } else if (node.callee == "print") {
      // TODO
    } else {
      throw std::runtime_error("Couldn't find symbol for " + node.callee);
    }
  };

  void visit(LetStatementNode &node) override {
    if (node.type) {
      node.type->accept(*this);
    }
    node.expression->accept(*this);

    auto type =
        node.type ? getType(node.type.get()) : getType(node.expression.get());
    Symbol symbol{.name = node.name, .type = type};

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
    for (const auto &property : node.properties) {
      property->initializer->accept(*this);
      properties.push_back(
          std::make_pair(property->name, getType(property->initializer.get())));
    }
    setType(node, std::make_shared<Type>(StructType{properties}));
  };

  void visit(ParameterNode &node) override {
    Symbol symbol{.name = node.name, .type = typeNodeToType(node.type.get())};
    currentScope->symbolTable.addSymbol(symbol);
  };

  void visit(ArrowFunctionExpressionNode &node) override {
    std::vector<std::shared_ptr<Type>> parameters;
    for (auto &parameter : node.parameters) {
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
    Symbol symbol{.name = node.name, .type = typeNodeToType(&node)};
    currentScope->symbolTable.addSymbol(symbol);
  }

  void visit(FunctionDeclarationNode &node) override {
    std::vector<std::shared_ptr<Type>> parameters;
    for (auto &parameter : node.parameters) {
      parameters.push_back(typeNodeToType(parameter->type.get()));
    }
    auto functionType = std::make_shared<Type>(FunctionType{
        typeNodeToType(node.returnType.get()), std::move(parameters)});
    Symbol symbol{.name = node.name, .type = std::move(functionType)};
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

  void createSymbolsFromSourceFile(SourceFileNode &node) { node.accept(*this); }

  void print() { globalScope.print(0); }
};