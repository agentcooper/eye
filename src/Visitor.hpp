#pragma once

struct ArrowFunctionExpressionNode;
struct BinaryExpressionNode;
struct BlockNode;
struct BooleanLiteralNode;
struct CallExpressionNode;
struct CharLiteralNode;
struct ElementAccessExpressionNode;
struct ExpressionStatementNode;
struct ForStatementNode;
struct FunctionDeclarationNode;
struct FunctionTypeNode;
struct IdentifierNode;
struct IfStatementNode;
struct InterfaceDeclarationNode;
struct LetStatementNode;
struct LiteralTypeNode;
struct NumericLiteralNode;
struct ObjectLiteralNode;
struct ParameterNode;
struct PropertyAssignmentNode;
struct PropertySignatureNode;
struct ReturnStatementNode;
struct SourceFileNode;
struct StringLiteralNode;
struct StructTypeNode;
struct TypeReferenceNode;
struct UnaryExpressionNode;

class Visitor {
public:
  virtual void visit(ArrowFunctionExpressionNode &node) = 0;
  virtual void visit(BinaryExpressionNode &node) = 0;
  virtual void visit(BlockNode &node) = 0;
  virtual void visit(BooleanLiteralNode &node) = 0;
  virtual void visit(CallExpressionNode &node) = 0;
  virtual void visit(CharLiteralNode &node) = 0;
  virtual void visit(ElementAccessExpressionNode &node) = 0;
  virtual void visit(ExpressionStatementNode &node) = 0;
  virtual void visit(ForStatementNode &node) = 0;
  virtual void visit(FunctionDeclarationNode &node) = 0;
  virtual void visit(FunctionTypeNode &node) = 0;
  virtual void visit(IdentifierNode &node) = 0;
  virtual void visit(IfStatementNode &node) = 0;
  virtual void visit(InterfaceDeclarationNode &node) = 0;
  virtual void visit(LetStatementNode &node) = 0;
  virtual void visit(LiteralTypeNode &node) = 0;
  virtual void visit(NumericLiteralNode &node) = 0;
  virtual void visit(ObjectLiteralNode &node) = 0;
  virtual void visit(ParameterNode &node) = 0;
  virtual void visit(PropertyAssignmentNode &node) = 0;
  virtual void visit(PropertySignatureNode &node) = 0;
  virtual void visit(ReturnStatementNode &node) = 0;
  virtual void visit(SourceFileNode &node) = 0;
  virtual void visit(StringLiteralNode &node) = 0;
  virtual void visit(StructTypeNode &node) = 0;
  virtual void visit(TypeReferenceNode &node) = 0;
  virtual void visit(UnaryExpressionNode &node) = 0;

  virtual ~Visitor() = default;
};