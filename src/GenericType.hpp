#pragma once

#include <filesystem>

#include "File.hpp"
#include "Node.hpp"
#include "SymbolTable.hpp"
#include "Type.hpp"

class GenericTypeVisitor : public Visitor {
public:
  std::unique_ptr<Node> value;

  GenericTypeVisitor(std::shared_ptr<Type> type) : type(type){};

private:
  std::shared_ptr<Type> type;

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

  void visit(BooleanLiteralNode &node) override {
    value = std::make_unique<BooleanLiteralNode>(node.value);
  }

  void visit(LiteralTypeNode &node) override {
    value = std::make_unique<LiteralTypeNode>(std::move(node.literal));
  }

  void visit(CharLiteralNode &node) override {
    value = std::make_unique<CharLiteralNode>(node.value);
  }

  void visit(StringLiteralNode &node) override {
    value = std::make_unique<StringLiteralNode>(node.text);
  }

  void visit(ObjectLiteralNode &node) override {
    value = std::make_unique<ObjectLiteralNode>(std::move(node.properties));
  }

  void visit(TypeReferenceNode &node) override {
    value = std::make_unique<TypeReferenceNode>(visit(node.typeName));
  };

  void visit(FunctionTypeNode &node) override {
    value = std::make_unique<FunctionTypeNode>(std::move(node.parameters),
                                               visit(node.returnType));
  };

  void visit(IdentifierNode &node) override {
    if (node.name == "T") {
      value = std::make_unique<IdentifierNode>(typeToString(*type));
      return;
    }

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

  void visit(ForStatementNode &node) override {
    value = std::make_unique<ForStatementNode>(
        visit(node.initializer), visit(node.condition), visit(node.incrementer),
        visit(node.body));
  };

  void visit(ElementAccessExpressionNode &node) override {
    value = std::make_unique<ElementAccessExpressionNode>(
        visit(node.expression), visit(node.argumentExpression));
  };

  void visit(UnaryExpressionNode &node) override {
    value =
        std::make_unique<UnaryExpressionNode>(node.op, visit(node.expression));
  }

  void visit(BinaryExpressionNode &node) override {
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
    value = std::make_unique<ArrowFunctionExpressionNode>(
        std::move(node.parameters), visit(node.returnType), visit(node.body));
  };

  void visit(FunctionDeclarationNode &node) override {
    // TODO: support variable number of parameters
    std::vector<std::string> emptyTypeParameters;
    value = std::make_unique<FunctionDeclarationNode>(
        node.name + "_" + typeToString(*type) + "_" + typeToString(*type),
        emptyTypeParameters, visit(node.parameters), visit(node.returnType),
        visit(node.body));
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

std::unique_ptr<Node>
instantiateGenericTypeParameter(Node &node, std::shared_ptr<Type> type) {
  GenericTypeVisitor visitor{type};
  node.accept(visitor);
  return std::move(visitor.value);
}