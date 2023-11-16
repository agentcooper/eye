#pragma once

#include "Node.hpp"
#include "SymbolTable.hpp"
#include "Type.hpp"

class PostProcessVisitor : public Visitor {
public:
  std::unique_ptr<Node> value;

  PostProcessVisitor(SymbolTableVisitor &symbolTableVisitor)
      : symbolTableVisitor(symbolTableVisitor){};

private:
  SymbolTableVisitor &symbolTableVisitor;
  size_t arrowFunctionExpressionIndex = 0;

  std::vector<std::unique_ptr<Node>>
  visit(std::vector<std::unique_ptr<Node>> &nodes) {
    std::vector<std::unique_ptr<Node>> result;
    for (auto &node : nodes) {
      result.push_back(visit(node));
    }
    return result;
  }

  std::unique_ptr<Node> visit(std::unique_ptr<Node> &node) {
    if (node == nullptr) {
      return nullptr;
    }
    node->accept(*this);
    return std::move(value);
  }

  void visit(NumericLiteralNode &node) override {
    value =
        std::make_unique<NumericLiteralNode>(node.value, node.hasFloatingPoint);
  }

  void visit(StringLiteralNode &node) override {
    value = std::make_unique<StringLiteralNode>(node.text);
  }

  void visit(ObjectLiteralNode &node) override {
    value = std::make_unique<ObjectLiteralNode>(std::move(node.properties));
  }

  void visit(TypeReferenceNode &node) override {
    value = std::make_unique<TypeReferenceNode>(std::move(node.typeName));
  };

  void visit(FunctionTypeNode &node) override {
    value = std::make_unique<FunctionTypeNode>(std::move(node.parameters),
                                               visit(node.returnType));
  };

  void visit(IdentifierNode &node) override {
    value = std::make_unique<IdentifierNode>(node.name);
  };

  void visit(ReturnStatementNode &node) override {
    value = std::make_unique<ReturnStatementNode>(visit(node.expression));
  };

  void visit(ExpressionStatementNode &node) override {
    value = std::make_unique<ExpressionStatementNode>(visit(node.expression));
  };

  void visit(IfStatementNode &node) override {
    value = std::make_unique<IfStatementNode>(
        visit(node.condition), visit(node.ifTrue), visit(node.ifFalse));
  };

  void visit(LetStatementNode &node) override {
    value = std::make_unique<LetStatementNode>(node.name, std::move(node.type),
                                               visit(node.expression));
  };

  void visit(BinaryExpressionNode &node) override {
    if (node.op == Token::Kind::Plus) {
      auto t1 = symbolTableVisitor.inferType(node.lhs.get());
      auto t2 = symbolTableVisitor.inferType(node.rhs.get());

      if (isStringType(*t1) && isStringType(*t2)) {
        std::vector<std::unique_ptr<Node>> arguments;
        arguments.push_back(visit(node.lhs));
        arguments.push_back(visit(node.rhs));
        value = std::make_unique<CallExpressionNode>("joinStrings",
                                                     std::move(arguments));
        return;
      }
    }

    value = std::make_unique<BinaryExpressionNode>(node.op, visit(node.lhs),
                                                   visit(node.rhs));
  };

  void visit(CallExpressionNode &node) override {
    value = std::make_unique<CallExpressionNode>(node.callee,
                                                 visit(node.arguments));
  };

  void visit(BlockNode &node) override {
    value = std::make_unique<BlockNode>(visit(node.statements));
  };

  void visit(PropertySignatureNode &node) override {
    value =
        std::make_unique<PropertyAssignmentNode>(node.name, visit(node.type));
  };

  void visit(PropertyAssignmentNode &node) override {
    value = std::make_unique<PropertyAssignmentNode>(node.name,
                                                     visit(node.initializer));
  };

  void visit(ParameterNode &node) override {
    value = std::make_unique<ParameterNode>(node.name, visit(node.type));
  };

  void visit(ArrowFunctionExpressionNode &node) override {
    std::string scopeName =
        "anonymous" + std::to_string(arrowFunctionExpressionIndex++);

    symbolTableVisitor.enterScope(scopeName);

    value = std::make_unique<ArrowFunctionExpressionNode>(
        std::move(node.parameters), visit(node.returnType), visit(node.body));

    symbolTableVisitor.exitScope();
  };

  void visit(FunctionDeclarationNode &node) override {
    symbolTableVisitor.enterScope(node.name);

    value = std::make_unique<FunctionDeclarationNode>(
        node.name, std::move(node.parameters), std::move(node.returnType),
        visit(node.body));

    symbolTableVisitor.exitScope();
  };

  void visit(StructTypeNode &node) override {
    value = std::make_unique<StructTypeNode>(std::move(node.members));
  }

  void visit(InterfaceDeclarationNode &node) override {
    value = std::make_unique<InterfaceDeclarationNode>(node.name,
                                                       std::move(node.members));
  };

  void visit(SourceFileNode &node) override {
    value = std::make_unique<SourceFileNode>(visit(node.functions),
                                             visit(node.interfaces));
  };
};

class PostProcessor {
  Node &node;
  SymbolTableVisitor &symbolTableVisitor;

public:
  PostProcessor(Node &node, SymbolTableVisitor &symbolTableVisitor)
      : node(node), symbolTableVisitor(symbolTableVisitor) {}

  std::unique_ptr<SourceFileNode> run() {
    PostProcessVisitor postProcessVisitor{symbolTableVisitor};
    node.accept(postProcessVisitor);
    std::unique_ptr<SourceFileNode> sourceFileNode(
        static_cast<SourceFileNode *>(postProcessVisitor.value.release()));
    return sourceFileNode;
  }
};