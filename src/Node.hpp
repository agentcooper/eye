#pragma once

#include <string>

#include "LLVM.hpp"
#include "Token.hpp"
#include "Visitor.hpp"

struct Node {
public:
  virtual ~Node() = default;

  virtual std::unique_ptr<Node> clone() const = 0;
  virtual void accept(class Visitor &v) = 0;
};

std::unique_ptr<Node> cloneNode(const std::unique_ptr<Node> &node);

std::vector<std::unique_ptr<Node>>
cloneNodes(const std::vector<std::unique_ptr<Node>> &nodes);

struct LiteralTypeNode : public Node {
  std::unique_ptr<Node> literal;

  LiteralTypeNode(std::unique_ptr<Node> literal)
      : literal(std::move(literal)) {}

  std::unique_ptr<Node> clone() const override {
    return std::make_unique<LiteralTypeNode>(cloneNode(literal));
  }

  void accept(Visitor &v) override { v.visit(*this); }
};

struct TypeReferenceNode : public Node {
  std::unique_ptr<Node> typeName;
  std::vector<std::unique_ptr<Node>> typeParameters;

  TypeReferenceNode(std::unique_ptr<Node> typeName)
      : typeName(std::move(typeName)) {}
  TypeReferenceNode(std::unique_ptr<Node> typeName,
                    std::vector<std::unique_ptr<Node>> typeParameters)
      : typeName(std::move(typeName)),
        typeParameters(std::move(typeParameters)) {}

  std::unique_ptr<Node> clone() const override {
    if (typeParameters.empty()) {
      return std::make_unique<TypeReferenceNode>(cloneNode(typeName));
    }
    return std::make_unique<TypeReferenceNode>(cloneNode(typeName),
                                               cloneNodes(typeParameters));
  }

  void accept(Visitor &v) override { v.visit(*this); }
};

struct StructTypeNode : public Node {
  std::vector<std::unique_ptr<Node>> members;

  StructTypeNode(std::vector<std::unique_ptr<Node>> members)
      : members(std::move(members)) {}

  std::unique_ptr<Node> clone() const override {
    return std::make_unique<StructTypeNode>(cloneNodes(members));
  }

  void accept(Visitor &v) override { v.visit(*this); }
};

struct FunctionTypeNode : public Node {
  std::vector<std::unique_ptr<Node>> parameters;
  std::unique_ptr<Node> returnType;

  FunctionTypeNode(std::vector<std::unique_ptr<Node>> parameters,
                   std::unique_ptr<Node> returnType)
      : parameters(std::move(parameters)), returnType(std::move(returnType)) {}

  std::unique_ptr<Node> clone() const override {
    return std::make_unique<FunctionTypeNode>(cloneNodes(parameters),
                                              cloneNode(returnType));
  }

  void accept(Visitor &v) override { v.visit(*this); }
};

struct BooleanLiteralNode : public Node {
  bool value;
  BooleanLiteralNode(bool value) : value(value) {}

  std::unique_ptr<Node> clone() const override {
    return std::make_unique<BooleanLiteralNode>(value);
  }

  void accept(Visitor &v) override { v.visit(*this); }
};

struct NumericLiteralNode : public Node {
  const double value;
  bool hasFloatingPoint;
  NumericLiteralNode(const double value, const bool hasFloatingPoint)
      : value(value), hasFloatingPoint(hasFloatingPoint) {}

  std::unique_ptr<Node> clone() const override {
    return std::make_unique<NumericLiteralNode>(value, hasFloatingPoint);
  }

  void accept(Visitor &v) override { v.visit(*this); }
};

struct IdentifierNode : public Node {
  const std::string name;
  IdentifierNode(const std::string &name) : name(name) {}

  std::unique_ptr<Node> clone() const override {
    return std::make_unique<IdentifierNode>(name);
  }

  void accept(Visitor &v) override { v.visit(*this); }
};

struct CharLiteralNode : public Node {
  const char value;
  CharLiteralNode(const char &value) : value(value) {}

  std::unique_ptr<Node> clone() const override {
    return std::make_unique<CharLiteralNode>(value);
  }

  void accept(Visitor &v) override { v.visit(*this); }
};

struct StringLiteralNode : public Node {
  const std::string text;
  StringLiteralNode(const std::string &text) : text(text) {}

  std::unique_ptr<Node> clone() const override {
    return std::make_unique<StringLiteralNode>(text);
  }

  void accept(Visitor &v) override { v.visit(*this); }
};

struct ParameterNode : public Node {
  const std::string name;
  std::unique_ptr<Node> type;
  ParameterNode(const std::string &name, std::unique_ptr<Node> type)
      : name(name), type(std::move(type)) {}

  std::unique_ptr<Node> clone() const override {
    return std::make_unique<ParameterNode>(name, cloneNode(type));
  }

  void accept(Visitor &v) override { v.visit(*this); }
};

struct PropertySignatureNode : public Node {
  const std::string name;
  std::unique_ptr<Node> type;
  PropertySignatureNode(const std::string &name, std::unique_ptr<Node> type)
      : name(name), type(std::move(type)) {}

  std::unique_ptr<Node> clone() const override {
    return std::make_unique<PropertySignatureNode>(name, cloneNode(type));
  }

  void accept(Visitor &v) override { v.visit(*this); }
};

struct PropertyAssignmentNode : public Node {
  const std::string name;
  std::unique_ptr<Node> initializer;
  PropertyAssignmentNode(const std::string &name,
                         std::unique_ptr<Node> initializer)
      : name(name), initializer(std::move(initializer)) {}

  std::unique_ptr<Node> clone() const override {
    return std::make_unique<PropertyAssignmentNode>(name,
                                                    cloneNode(initializer));
  }

  void accept(Visitor &v) override { v.visit(*this); }
};

struct ReturnStatementNode : public Node {
  std::unique_ptr<Node> expression;
  ReturnStatementNode(std::unique_ptr<Node> expression)
      : expression(std::move(expression)) {}

  std::unique_ptr<Node> clone() const override {
    return std::make_unique<ReturnStatementNode>(cloneNode(expression));
  }

  void accept(Visitor &v) override { v.visit(*this); }
};

struct ExpressionStatementNode : public Node {
  std::unique_ptr<Node> expression;
  ExpressionStatementNode(std::unique_ptr<Node> expression)
      : expression(std::move(expression)) {}

  std::unique_ptr<Node> clone() const override {
    return std::make_unique<ExpressionStatementNode>(cloneNode(expression));
  }

  void accept(Visitor &v) override { v.visit(*this); }
};

struct IfStatementNode : public Node {
  std::unique_ptr<Node> condition;
  std::unique_ptr<Node> ifTrue;
  std::unique_ptr<Node> ifFalse;
  IfStatementNode(std::unique_ptr<Node> condition, std::unique_ptr<Node> ifTrue,
                  std::unique_ptr<Node> ifFalse)
      : condition(std::move(condition)), ifTrue(std::move(ifTrue)),
        ifFalse(std::move(ifFalse)) {}

  std::unique_ptr<Node> clone() const override {
    return std::make_unique<IfStatementNode>(
        cloneNode(condition), cloneNode(ifTrue), cloneNode(ifFalse));
  }

  void accept(Visitor &v) override { v.visit(*this); }
};

struct LetStatementNode : public Node {
  const std::string name;
  std::unique_ptr<Node> type;
  std::unique_ptr<Node> expression;

  LetStatementNode(const std::string &name, std::unique_ptr<Node> type,
                   std::unique_ptr<Node> expression)
      : name(name), type(std::move(type)), expression(std::move(expression)) {}

  std::unique_ptr<Node> clone() const override {
    return std::make_unique<LetStatementNode>(name, cloneNode(type),
                                              cloneNode(expression));
  }

  void accept(Visitor &v) override { v.visit(*this); }
};

struct ForStatementNode : public Node {
  std::unique_ptr<Node> initializer;
  std::unique_ptr<Node> condition;
  std::unique_ptr<Node> incrementer;
  std::unique_ptr<Node> body;

  ForStatementNode(std::unique_ptr<Node> initializer,
                   std::unique_ptr<Node> condition,
                   std::unique_ptr<Node> incrementer,
                   std::unique_ptr<Node> body)
      : initializer(std::move(initializer)), condition(std::move(condition)),
        incrementer(std::move(incrementer)), body(std::move(body)) {}

  std::unique_ptr<Node> clone() const override {
    return std::make_unique<ForStatementNode>(
        cloneNode(initializer), cloneNode(condition), cloneNode(incrementer),
        cloneNode(body));
  }

  void accept(Visitor &v) override { v.visit(*this); }
};

struct ElementAccessExpressionNode : public Node {
  std::unique_ptr<Node> expression;
  std::unique_ptr<Node> argumentExpression;

  ElementAccessExpressionNode(std::unique_ptr<Node> expression,
                              std::unique_ptr<Node> argumentExpression)
      : expression(std::move(expression)),
        argumentExpression(std::move(argumentExpression)) {}

  std::unique_ptr<Node> clone() const override {
    return std::make_unique<ElementAccessExpressionNode>(
        cloneNode(expression), cloneNode(argumentExpression));
  }

  void accept(Visitor &v) override { v.visit(*this); }
};

struct UnaryExpressionNode : public Node {
  Token::Kind op;
  std::unique_ptr<Node> expression;

  UnaryExpressionNode(Token::Kind op, std::unique_ptr<Node> expression)
      : op(op), expression(std::move(expression)) {}

  std::unique_ptr<Node> clone() const override {
    return std::make_unique<UnaryExpressionNode>(op, cloneNode(expression));
  }

  void accept(Visitor &v) override { v.visit(*this); }
};

struct BinaryExpressionNode : public Node {
  Token::Kind op;
  std::unique_ptr<Node> lhs;
  std::unique_ptr<Node> rhs;

  BinaryExpressionNode(Token::Kind op, std::unique_ptr<Node> lhs,
                       std::unique_ptr<Node> rhs)
      : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

  std::unique_ptr<Node> clone() const override {
    return std::make_unique<BinaryExpressionNode>(op, cloneNode(lhs),
                                                  cloneNode(rhs));
  }

  void accept(Visitor &v) override { v.visit(*this); }
};

struct CallExpressionNode : public Node {
  const std::string callee;
  std::vector<std::unique_ptr<Node>> arguments;

  CallExpressionNode(const std::string &callee,
                     std::vector<std::unique_ptr<Node>> arguments)
      : callee(callee), arguments(std::move(arguments)) {}

  std::unique_ptr<Node> clone() const override {
    return std::make_unique<CallExpressionNode>(callee, cloneNodes(arguments));
  }

  void accept(Visitor &v) override { v.visit(*this); }
};

struct BlockNode : public Node {
  std::vector<std::unique_ptr<Node>> statements;

  BlockNode(std::vector<std::unique_ptr<Node>> statements)
      : statements(std::move(statements)){};

  std::unique_ptr<Node> clone() const override {
    return std::make_unique<BlockNode>(cloneNodes(statements));
  }

  void accept(Visitor &v) override { v.visit(*this); }
};

struct ArrowFunctionExpressionNode : public Node {
  std::vector<std::unique_ptr<Node>> parameters;
  std::unique_ptr<Node> returnType;
  std::unique_ptr<Node> body;

  ArrowFunctionExpressionNode(std::vector<std::unique_ptr<Node>> parameters,
                              std::unique_ptr<Node> returnType,
                              std::unique_ptr<Node> body)
      : parameters(std::move(parameters)), returnType(std::move(returnType)),
        body(std::move(body)) {}

  std::unique_ptr<Node> clone() const override {
    return std::make_unique<ArrowFunctionExpressionNode>(
        cloneNodes(parameters), cloneNode(returnType), cloneNode(body));
  }

  void accept(Visitor &v) override { v.visit(*this); }
};

struct ObjectLiteralNode : public Node {
  std::vector<std::unique_ptr<Node>> properties;

  ObjectLiteralNode(std::vector<std::unique_ptr<Node>> properties)
      : properties(std::move(properties)) {}

  std::unique_ptr<Node> clone() const override {
    return std::make_unique<ObjectLiteralNode>(cloneNodes(properties));
  }

  void accept(Visitor &v) override { v.visit(*this); }
};

struct InterfaceDeclarationNode : public Node {
  const std::string name;
  std::vector<std::unique_ptr<Node>> members;

  InterfaceDeclarationNode(const std::string &name,
                           std::vector<std::unique_ptr<Node>> members)
      : name(name), members(std::move(members)) {}

  std::unique_ptr<Node> clone() const override {
    return std::make_unique<InterfaceDeclarationNode>(name,
                                                      cloneNodes(members));
  }

  void accept(Visitor &v) override { v.visit(*this); }
};

struct FunctionDeclarationNode : public Node {
  const std::string name;
  bool isGeneric;

  std::vector<std::string> typeParameters;
  std::vector<std::unique_ptr<Node>> parameters;
  std::unique_ptr<Node> returnType;
  std::unique_ptr<Node> body;

  FunctionDeclarationNode(const std::string &name,
                          std::vector<std::string> typeParameters,
                          std::vector<std::unique_ptr<Node>> parameters,
                          std::unique_ptr<Node> returnType,
                          std::unique_ptr<Node> body)
      : name(name), typeParameters(typeParameters),
        parameters(std::move(parameters)), returnType(std::move(returnType)),
        body(std::move(body)) {}

  std::unique_ptr<Node> clone() const override {
    auto typeParametersCopy = typeParameters;
    return std::make_unique<FunctionDeclarationNode>(
        name, typeParametersCopy, cloneNodes(parameters), cloneNode(returnType),
        cloneNode(body));
  }

  void accept(Visitor &v) override { v.visit(*this); }
};

struct SourceFileNode : public Node {
  std::vector<std::unique_ptr<Node>> functions;
  std::vector<std::unique_ptr<Node>> interfaces;

  SourceFileNode(std::vector<std::unique_ptr<Node>> functions,
                 std::vector<std::unique_ptr<Node>> interfaces)
      : functions(std::move(functions)), interfaces(std::move(interfaces)) {}

  std::unique_ptr<Node> clone() const override {
    return std::make_unique<SourceFileNode>(cloneNodes(functions),
                                            cloneNodes(interfaces));
  }

  void accept(Visitor &v) override { v.visit(*this); }
};