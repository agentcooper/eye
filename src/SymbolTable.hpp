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
public:
  SourceFileNode &sourceFileNode;

  Scope globalScope;
  Scope *currentScope;

  size_t arrowFunctionExpressionIndex = 0;

  void enterScope(std::string name) {
    auto it =
        std::find_if(currentScope->scopes.begin(), currentScope->scopes.end(),
                     [&name](auto x) { return x->name == name; });
    if (it == currentScope->scopes.end()) {
      throw std::runtime_error("Could not find scope");
    }
    currentScope = (*it).get();
  }

  void createScopeAndEnter(std::string name) {
    currentScope = currentScope->createChildScope(name);
  }

  void exitScope() { currentScope = currentScope->parent; }

  std::shared_ptr<Type> inferType(Node *node) const {
    auto *callExpression = dynamic_cast<CallExpressionNode *>(node);
    if (callExpression) {
      if (auto symbol = currentScope->lookup(callExpression->callee)) {
        if (FunctionType *functionType =
                std::get_if<FunctionType>(&*symbol->type)) {
          return functionType->returnType;
        }
      }
    }

    auto *arrowFunctionExpression =
        dynamic_cast<ArrowFunctionExpressionNode *>(node);
    if (arrowFunctionExpression) {
      std::vector<std::shared_ptr<Type>> parameters;
      for (auto &parameter : arrowFunctionExpression->parameters) {
        parameters.push_back(typeNodeToType(parameter->type.get()));
      }
      auto functionType = std::make_shared<Type>(FunctionType{
          typeNodeToType(arrowFunctionExpression->returnType.get()),
          std::move(parameters)});
      return functionType;
    }

    auto *identifier = dynamic_cast<IdentifierNode *>(node);
    if (identifier) {
      if (auto symbol = currentScope->lookup(identifier->name)) {
        if (TypeReference *typeReference =
                std::get_if<TypeReference>(&*symbol->type)) {
          return currentScope->lookup(typeReference->name)->type;
        }
        return symbol->type;
      } else {
        std::cout << "Got identifier, but could not look it up" << std::endl;
      }
    }

    auto *typeReference = dynamic_cast<TypeReferenceNode *>(node);
    if (typeReference) {
      if (auto symbol = currentScope->lookup(typeReference->typeName->name)) {
        return symbol->type;
      }
      throw std::runtime_error("Couldn't find the symbol");
    }

    auto *numericLiteral = dynamic_cast<NumericLiteralNode *>(node);
    if (numericLiteral) {
      if (numericLiteral->hasFloatingPoint) {
        return std::make_shared<Type>(PrimitiveType::f64Type);
      }
      return std::make_shared<Type>(PrimitiveType::i64Type);
    }

    auto *stringLiteral = dynamic_cast<StringLiteralNode *>(node);
    if (stringLiteral) {
      return std::make_shared<Type>(PrimitiveType::stringType);
    }

    auto *objectLiteral = dynamic_cast<ObjectLiteralNode *>(node);
    if (objectLiteral) {
      std::vector<NamedType> properties;
      for (const auto &property : objectLiteral->properties) {
        properties.push_back(std::make_pair(
            property->name, inferType(property->initializer.get())));
      }
      return std::make_shared<Type>(StructType{properties});
    }

    auto *binaryExpression = dynamic_cast<BinaryExpressionNode *>(node);
    if (binaryExpression) {
      if (binaryExpression->op != Token::Kind::Dot) {
        throw std::runtime_error("Expected op = dot");
      }

      auto *rhsIdentifier =
          dynamic_cast<IdentifierNode *>(binaryExpression->rhs.get());
      if (!rhsIdentifier) {
        throw std::runtime_error("Expected identifier as rhs");
      }

      auto lhsType = inferType(binaryExpression->lhs.get());
      StructType *structType = std::get_if<StructType>(&*lhsType);
      if (!structType) {
        throw std::runtime_error("2 Expected struct type, but got " +
                                 typeToString(*lhsType));
      }
      for (const auto &property : structType->properties) {
        if (property.first == rhsIdentifier->name) {
          return property.second;
        }
      }
    }

    return std::make_shared<Type>(PrimitiveType::unknownType);
  }

public:
  SymbolTableVisitor(SourceFileNode &sourceFileNode)
      : sourceFileNode(sourceFileNode), globalScope("global", nullptr),
        currentScope(&globalScope) {}

  void visit(TypeReferenceNode &node) override {}

  void visit(FunctionTypeNode &node) override {}

  void visit(NumericLiteralNode &node) override {}

  void visit(StringLiteralNode &node) override {}

  void visit(IdentifierNode &node) override{};

  void visit(ReturnStatementNode &node) override {
    node.expression->accept(*this);
  };

  void visit(ExpressionStatementNode &node) override {
    node.expression->accept(*this);
  };

  void visit(IfStatementNode &node) override{};

  void visit(BinaryExpressionNode &node) override{};

  void visit(CallExpressionNode &node) override {
    for (const auto &argument : node.arguments) {
      argument->accept(*this);
    }
  };

  void visit(LetStatementNode &node) override {

    auto type = node.type ? inferType(node.type.get())
                          : inferType(node.expression.get());
    Symbol symbol{.name = node.name, .type = type};

    currentScope->symbolTable.addSymbol(symbol);

    node.expression->accept(*this);
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

  void visit(ObjectLiteralNode &node) override{
      // TODO
  };

  void visit(ParameterNode &node) override {
    Symbol symbol{.name = node.name, .type = typeNodeToType(node.type.get())};
    currentScope->symbolTable.addSymbol(symbol);
  };

  void visit(ArrowFunctionExpressionNode &node) override {
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

    node.body->accept(*this);
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

  void run() { sourceFileNode.accept(*this); }

  void print() { globalScope.print(0); }
};