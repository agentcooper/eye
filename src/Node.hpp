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
  TypeReferenceNode(std::unique_ptr<IdentifierNode> typeName)
      : typeName(std::move(typeName)) {}

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

struct NumericLiteralNode : public Node {
  const int value;
  NumericLiteralNode(const int value) : value(value) {}

  void accept(Visitor &v) override { v.visit(*this); }
};

struct IdentifierNode : public Node {
  const std::string name;
  IdentifierNode(const std::string &name) : name(name) {}

  void accept(Visitor &v) override { v.visit(*this); }
};

struct ParameterNode : public Node {
  const std::string name;
  std::unique_ptr<Node> type;
  ParameterNode(const std::string &name, std::unique_ptr<Node> type)
      : name(name), type(std::move(type)) {}

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
  std::unique_ptr<Node> expression;

  LetStatementNode(const std::string &name, std::unique_ptr<Node> expression)
      : name(name), expression(std::move(expression)) {}

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
  std::unique_ptr<BlockNode> body;

  ArrowFunctionExpressionNode(
      std::vector<std::unique_ptr<ParameterNode>> parameters,
      std::unique_ptr<Node> returnType, std::unique_ptr<BlockNode> body)
      : parameters(std::move(parameters)), returnType(std::move(returnType)),
        body(std::move(body)) {}

  void accept(Visitor &v) override { v.visit(*this); }
};

struct FunctionDeclarationNode : public Node {
  const std::string name;
  std::vector<std::unique_ptr<ParameterNode>> parameters;
  std::unique_ptr<Node> returnType;
  std::unique_ptr<BlockNode> body;

  FunctionDeclarationNode(
      const std::string &name,
      std::vector<std::unique_ptr<ParameterNode>> parameters,
      std::unique_ptr<Node> returnType, std::unique_ptr<BlockNode> body)
      : name(name), parameters(std::move(parameters)),
        returnType(std::move(returnType)), body(std::move(body)) {}

  void accept(Visitor &v) override { v.visit(*this); }
};

struct SourceFileNode : public Node {
  std::vector<std::unique_ptr<FunctionDeclarationNode>> functions;

  void accept(Visitor &v) override { v.visit(*this); }
};