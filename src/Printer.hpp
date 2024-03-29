#pragma once

#include <stack>

#include "Node.hpp"

class PrintVisitor : public Visitor {
  const bool isLast;
  std::string prefix;
  const std::string property;

public:
  PrintVisitor() : isLast(true), prefix(""), property("") {}

  PrintVisitor(bool isLast, std::string prefix, std::string property)
      : isLast(isLast), prefix(prefix), property(property) {}

private:
  void accept(Node &node, bool isLast, std::string property = "") {
    PrintVisitor p{isLast, prefix, property};
    node.accept(p);
  }

  void maybeAccept(Node *node, bool isLast, std::string property = "") {
    if (!node) {
      PrintVisitor p{isLast, prefix, property};
      p.printPrefix();
      std::cout << "Null" << std::endl;
      return;
    }
    accept(*node, isLast, property);
  }

  void printPrefix() {
    auto postfix = property.size() > 0 ? (property + ": ") : "";

    std::cout << prefix;
    if (isLast) {
      std::cout << "└─ " << postfix;
      prefix += "   " + std::string(postfix.size(), ' ');
    } else {
      std::cout << "├─ " << postfix;
      prefix += "│  " + std::string(postfix.size(), ' ');
    }
  }

  void visit(NumericLiteralNode &node) override {
    printPrefix();
    std::cout << "NumericLiteral(" << node.value << ")" << std::endl;
  }

  void visit(BooleanLiteralNode &node) override {
    printPrefix();
    std::cout << "BooleanLiteral(" << (node.value ? "true" : "false") << ")"
              << std::endl;
  }

  void visit(StringLiteralNode &node) override {
    printPrefix();
    std::cout << "StringLiteral(" << node.text << ")" << std::endl;
  }

  void visit(CharLiteralNode &node) override {
    printPrefix();

    std::cout << "CharLiteral("
              << (node.value == '\n' ? "\\n" : std::string(1, node.value))
              << ")" << std::endl;
  }

  void visit(ObjectLiteralNode &node) override {
    printPrefix();
    std::cout << "ObjectLiteral" << std::endl;
    for (const auto &property : node.properties) {
      auto isLast = &property == &node.properties.back();
      accept(*property, isLast);
    }
  }

  void visit(LiteralTypeNode &node) override {
    printPrefix();
    std::cout << "LiteralType" << std::endl;
    accept(*node.literal, true);
  };

  void visit(TypeReferenceNode &node) override {
    printPrefix();
    std::cout << "TypeReference" << std::endl;
    accept(*node.typeName, node.typeParameters.empty());
    for (const auto &typeParameter : node.typeParameters) {
      auto isLast = &typeParameter == &node.typeParameters.back();
      accept(*typeParameter, isLast);
    }
  };

  void visit(FunctionTypeNode &node) override {
    printPrefix();
    std::cout << "FunctionType" << std::endl;
    for (const auto &parameter : node.parameters) {
      accept(*parameter, false);
    }
    accept(*node.returnType, true, "returnType");
  };

  void visit(IdentifierNode &node) override {
    printPrefix();
    std::cout << "Identifier(" << node.name << ")" << std::endl;
  };

  void visit(ReturnStatementNode &node) override {
    printPrefix();
    std::cout << "ReturnStatement" << std::endl;
    maybeAccept(node.expression.get(), true, "expression");
  };

  void visit(ExpressionStatementNode &node) override {
    printPrefix();
    std::cout << "ExpressionStatement" << std::endl;
    accept(*node.expression, true, "expression");
  };

  void visit(IfStatementNode &node) override {
    printPrefix();
    std::cout << "IfStatement" << std::endl;
    accept(*node.condition, false, "condition");

    if (node.ifTrue && node.ifFalse) {
      accept(*node.ifTrue, false, "ifTrue");
      accept(*node.ifFalse, true, "ifFalse");
    } else {
      accept(*node.ifTrue, true, "ifTrue");
    }
  };

  void visit(LetStatementNode &node) override {
    printPrefix();
    std::cout << "LetStatement(" << node.name << ")" << std::endl;

    maybeAccept(node.type.get(), node.expression == nullptr, "type");
    maybeAccept(node.expression.get(), true, "expression");
  };

  void visit(ForStatementNode &node) override {
    printPrefix();
    std::cout << "ForStatement" << std::endl;

    accept(*node.initializer, false, "initializer");
    accept(*node.condition, false, "condition");
    accept(*node.incrementer, false, "incrementer");
    accept(*node.body, false, "body");
  };

  void visit(ElementAccessExpressionNode &node) override {
    printPrefix();
    std::cout << "ElementAccessExpression" << std::endl;
    accept(*node.expression, false, "expression");
    accept(*node.argumentExpression, true, "argument");
  };

  void visit(UnaryExpressionNode &node) override {
    printPrefix();
    std::cout << "UnaryExpression(" << kindToString(node.op) << ")"
              << std::endl;
    accept(*node.expression, true);
  };

  void visit(BinaryExpressionNode &node) override {
    printPrefix();
    std::cout << "BinaryExpression(" << kindToString(node.op) << ")"
              << std::endl;
    accept(*node.lhs, false, "lhs");
    accept(*node.rhs, true, "rhs");
  };

  void visit(CallExpressionNode &node) override {
    printPrefix();
    std::cout << "CallExpression(" << node.callee << ")" << std::endl;

    for (const auto &argument : node.arguments) {
      auto isLast = &argument == &node.arguments.back();
      accept(*argument, isLast);
    }
  };

  void visit(BlockNode &node) override {
    printPrefix();
    std::cout << "Block" << std::endl;
    for (const auto &statement : node.statements) {
      auto isLast = &statement == &node.statements.back();
      accept(*statement, isLast);
    }
  };

  void visit(PropertySignatureNode &node) override {
    printPrefix();
    std::cout << "PropertySignature(" << node.name << ")" << std::endl;
    accept(*node.type, true);
  };

  void visit(PropertyAssignmentNode &node) override {
    printPrefix();
    std::cout << "PropertyAssignment(" << node.name << ")" << std::endl;
    accept(*node.initializer, true);
  };

  void visit(ParameterNode &node) override {
    printPrefix();
    std::cout << "Parameter(" << node.name << ")" << std::endl;
    accept(*node.type, true);
  };

  void visit(ArrowFunctionExpressionNode &node) override {
    printPrefix();
    std::cout << "ArrowFunction" << std::endl;
    for (const auto &parameter : node.parameters) {
      accept(*parameter, false);
    }
    accept(*node.returnType, false, "returnType");
    accept(*node.body, true);
  };

  void visit(FunctionDeclarationNode &node) override {
    printPrefix();

    auto buildName = [&](FunctionDeclarationNode &node) {
      std::string result = node.name;
      if (!node.typeParameters.empty()) {
        result += "<";
        for (const auto &parameter : node.typeParameters) {
          result += parameter;
        }
        result += ">";
      }
      return result;
    };

    std::cout << "FunctionDeclaration(" << buildName(node) << ")" << std::endl;

    for (const auto &parameter : node.parameters) {
      accept(*parameter, false);
    }
    accept(*node.returnType, false, "returnType");
    if (node.body) {
      accept(*node.body, true);
    }
  };

  void visit(StructTypeNode &node) override {
    // TODO
    printPrefix();
    std::cout << "StructTypeNode" << std::endl;
    for (const auto &member : node.members) {
      auto isLast = &member == &node.members.back();
      accept(*member, isLast);
    }
  }

  void visit(InterfaceDeclarationNode &node) override {
    printPrefix();
    std::cout << "InterfaceDeclaration(" << node.name << ")" << std::endl;
    for (const auto &member : node.members) {
      auto isLast = &member == &node.members.back();
      accept(*member, isLast);
    }
  };

  void visit(SourceFileNode &node) override {
    std::cout << "SourceFile" << std::endl;
    for (const auto &interface : node.interfaces) {
      accept(*interface, false);
    }
    for (const auto &function : node.functions) {
      auto isLast = &function == &node.functions.back();
      accept(*function, isLast);
    }
  };
};

class Printer {
  Node &node;

public:
  Printer(Node &node) : node(node) {}

  void print() {
    PrintVisitor printVisitor;
    node.accept(printVisitor);
  }
};