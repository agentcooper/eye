#pragma once

#include <string>

#include "LLVM.hpp"
#include "Token.hpp"
#include "Visitor.hpp"

struct Node {
public:
  virtual ~Node() = default;

  virtual void accept(class Visitor &v) = 0;
};

struct TypeReferenceNode : public Node {
  std::unique_ptr<IdentifierNode> typeName;
  std::vector<std::unique_ptr<Node>> typeParameters;

  TypeReferenceNode(std::unique_ptr<IdentifierNode> typeName)
      : typeName(std::move(typeName)) {}
  TypeReferenceNode(std::unique_ptr<IdentifierNode> typeName,
                    std::vector<std::unique_ptr<Node>> typeParameters)
      : typeName(std::move(typeName)),
        typeParameters(std::move(typeParameters)) {}

  void accept(Visitor &v) override { v.visit(*this); }
};

struct StructTypeNode : public Node {
  std::vector<std::unique_ptr<PropertySignatureNode>> members;

  StructTypeNode(std::vector<std::unique_ptr<PropertySignatureNode>> members)
      : members(std::move(members)) {}

  void accept(Visitor &v) override { v.visit(*this); }
};

struct FunctionTypeNode : public Node {
  std::vector<std::unique_ptr<ParameterNode>> parameters;
  std::unique_ptr<Node> returnType;

  FunctionTypeNode(std::vector<std::unique_ptr<ParameterNode>> parameters,
                   std::unique_ptr<Node> returnType)
      : parameters(std::move(parameters)), returnType(std::move(returnType)) {}

  void accept(Visitor &v) override { v.visit(*this); }
};

struct BooleanLiteralNode : public Node {
  bool value;
  BooleanLiteralNode(bool value) : value(value) {}

  void accept(Visitor &v) override { v.visit(*this); }
};

struct NumericLiteralNode : public Node {
  const double value;
  bool hasFloatingPoint;
  NumericLiteralNode(const double value, const bool hasFloatingPoint)
      : value(value), hasFloatingPoint(hasFloatingPoint) {}

  void accept(Visitor &v) override { v.visit(*this); }
};

struct IdentifierNode : public Node {
  const std::string name;
  IdentifierNode(const std::string &name) : name(name) {}

  void accept(Visitor &v) override { v.visit(*this); }
};

struct CharLiteralNode : public Node {
  const char value;
  CharLiteralNode(const char &value) : value(value) {}

  void accept(Visitor &v) override { v.visit(*this); }
};

struct StringLiteralNode : public Node {
  const std::string text;
  StringLiteralNode(const std::string &text) : text(text) {}

  void accept(Visitor &v) override { v.visit(*this); }
};

struct ParameterNode : public Node {
  const std::string name;
  std::unique_ptr<Node> type;
  ParameterNode(const std::string &name, std::unique_ptr<Node> type)
      : name(name), type(std::move(type)) {}

  void accept(Visitor &v) override { v.visit(*this); }
};

struct PropertySignatureNode : public Node {
  const std::string name;
  std::unique_ptr<Node> type;
  PropertySignatureNode(const std::string &name, std::unique_ptr<Node> type)
      : name(name), type(std::move(type)) {}

  void accept(Visitor &v) override { v.visit(*this); }
};

struct PropertyAssignmentNode : public Node {
  const std::string name;
  std::unique_ptr<Node> initializer;
  PropertyAssignmentNode(const std::string &name,
                         std::unique_ptr<Node> initializer)
      : name(name), initializer(std::move(initializer)) {}

  void accept(Visitor &v) override { v.visit(*this); }
};

struct ReturnStatementNode : public Node {
  std::unique_ptr<Node> expression;
  ReturnStatementNode(std::unique_ptr<Node> expression)
      : expression(std::move(expression)) {}

  void accept(Visitor &v) override { v.visit(*this); }
};

struct ExpressionStatementNode : public Node {
  std::unique_ptr<Node> expression;
  ExpressionStatementNode(std::unique_ptr<Node> expression)
      : expression(std::move(expression)) {}

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

  void accept(Visitor &v) override { v.visit(*this); }
};

struct LetStatementNode : public Node {
  const std::string name;
  std::unique_ptr<Node> type;
  std::unique_ptr<Node> expression;

  LetStatementNode(const std::string &name, std::unique_ptr<Node> type,
                   std::unique_ptr<Node> expression)
      : name(name), type(std::move(type)), expression(std::move(expression)) {}

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

  void accept(Visitor &v) override { v.visit(*this); }
};

struct ElementAccessExpressionNode : public Node {
  std::unique_ptr<Node> expression;
  std::unique_ptr<Node> argumentExpression;

  ElementAccessExpressionNode(std::unique_ptr<Node> expression,
                              std::unique_ptr<Node> argumentExpression)
      : expression(std::move(expression)),
        argumentExpression(std::move(argumentExpression)) {}

  void accept(Visitor &v) override { v.visit(*this); }
};

struct UnaryExpressionNode : public Node {
  Token::Kind op;
  std::unique_ptr<Node> expression;

  UnaryExpressionNode(Token::Kind op, std::unique_ptr<Node> expression)
      : op(op), expression(std::move(expression)) {}

  void accept(Visitor &v) override { v.visit(*this); }
};

struct BinaryExpressionNode : public Node {
  Token::Kind op;
  std::unique_ptr<Node> lhs;
  std::unique_ptr<Node> rhs;

  BinaryExpressionNode(Token::Kind op, std::unique_ptr<Node> lhs,
                       std::unique_ptr<Node> rhs)
      : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

  void accept(Visitor &v) override { v.visit(*this); }
};

struct CallExpressionNode : public Node {
  const std::string callee;
  std::vector<std::unique_ptr<Node>> arguments;

  CallExpressionNode(const std::string &callee,
                     std::vector<std::unique_ptr<Node>> arguments)
      : callee(callee), arguments(std::move(arguments)) {}

  void accept(Visitor &v) override { v.visit(*this); }
};

struct BlockNode : public Node {
  std::vector<std::unique_ptr<Node>> statements;

  BlockNode(std::vector<std::unique_ptr<Node>> statements)
      : statements(std::move(statements)){};

  void accept(Visitor &v) override { v.visit(*this); }
};

struct ArrowFunctionExpressionNode : public Node {
  std::vector<std::unique_ptr<ParameterNode>> parameters;
  std::unique_ptr<Node> returnType;
  std::unique_ptr<Node> body;

  ArrowFunctionExpressionNode(
      std::vector<std::unique_ptr<ParameterNode>> parameters,
      std::unique_ptr<Node> returnType, std::unique_ptr<Node> body)
      : parameters(std::move(parameters)), returnType(std::move(returnType)),
        body(std::move(body)) {}

  void accept(Visitor &v) override { v.visit(*this); }
};

struct ObjectLiteralNode : public Node {
  std::vector<std::unique_ptr<PropertyAssignmentNode>> properties;

  ObjectLiteralNode(
      std::vector<std::unique_ptr<PropertyAssignmentNode>> properties)
      : properties(std::move(properties)) {}

  void accept(Visitor &v) override { v.visit(*this); }
};

struct InterfaceDeclarationNode : public Node {
  const std::string name;
  std::vector<std::unique_ptr<PropertySignatureNode>> members;

  InterfaceDeclarationNode(
      const std::string &name,
      std::vector<std::unique_ptr<PropertySignatureNode>> members)
      : name(name), members(std::move(members)) {}

  void accept(Visitor &v) override { v.visit(*this); }
};

struct FunctionDeclarationNode : public Node {
  const std::string name;
  std::vector<std::unique_ptr<ParameterNode>> parameters;
  std::unique_ptr<Node> returnType;
  std::unique_ptr<Node> body;

  FunctionDeclarationNode(
      const std::string &name,
      std::vector<std::unique_ptr<ParameterNode>> parameters,
      std::unique_ptr<Node> returnType, std::unique_ptr<Node> body)
      : name(name), parameters(std::move(parameters)),
        returnType(std::move(returnType)), body(std::move(body)) {}

  void accept(Visitor &v) override { v.visit(*this); }
};

struct SourceFileNode : public Node {
  std::vector<std::unique_ptr<Node>> functions;
  std::vector<std::unique_ptr<Node>> interfaces;

  SourceFileNode(std::vector<std::unique_ptr<Node>> functions,
                 std::vector<std::unique_ptr<Node>> interfaces)
      : functions(std::move(functions)), interfaces(std::move(interfaces)) {}

  void accept(Visitor &v) override { v.visit(*this); }
};