#include "Type.hpp"

struct TypePrinterVisitor {
  std::string operator()(const PrimitiveType &type) {
    switch (type) {
    case PrimitiveType::i64Type:
      return "i64";
    case PrimitiveType::f64Type:
      return "f64";
    case PrimitiveType::voidType:
      return "void";
    case PrimitiveType::unknownType:
      return "unknown";
    }
  }
  std::string operator()(const FunctionType &type) {
    std::string result;
    result += "(";
    for (const auto &parameter : type.parameters) {
      result += std::visit(*this, *parameter);
      if (parameter != type.parameters.back()) {
        result += ", ";
      }
    }
    result += ")";
    result += " => " + std::visit(*this, *type.returnType);
    return result;
  }
};

std::string typeToString(Type &type) {
  TypePrinterVisitor visitor;
  return std::visit(visitor, type);
}

std::shared_ptr<Type> typeNodeToType(Node *node) {
  auto typeReferenceNode = dynamic_cast<TypeReferenceNode *>(node);
  if (typeReferenceNode) {
    if (typeReferenceNode->typeName->name == "i64") {
      return std::make_shared<Type>(PrimitiveType::i64Type);
    }
    if (typeReferenceNode->typeName->name == "f64") {
      return std::make_shared<Type>(PrimitiveType::f64Type);
    }
    if (typeReferenceNode->typeName->name == "void") {
      return std::make_shared<Type>(PrimitiveType::voidType);
    }
    throw std::runtime_error(
        "Could not convert type reference to a primitive type");
  }

  auto functionTypeNode = dynamic_cast<FunctionTypeNode *>(node);
  if (functionTypeNode) {
    std::vector<std::shared_ptr<Type>> parameters;
    for (const auto &paramater : functionTypeNode->parameters) {
      parameters.push_back(typeNodeToType(paramater->type.get()));
    }
    return std::make_shared<Type>(
        FunctionType{typeNodeToType(functionTypeNode->returnType.get()),
                     std::move(parameters)});
  }

  throw std::runtime_error("Could not convert type node to a type");
}