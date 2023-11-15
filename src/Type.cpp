#include "Type.hpp"

struct TypePrinterVisitor {
  std::string operator()(const PrimitiveType &type) {
    switch (type) {
    case PrimitiveType::i64Type:
      return "i64";
    case PrimitiveType::f64Type:
      return "f64";
    case PrimitiveType::stringType:
      return "string";
    case PrimitiveType::voidType:
      return "void";
    case PrimitiveType::unknownType:
      return "unknown";
    }
  }
  std::string operator()(const TypeReference &type) { return type.name; }
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
  std::string operator()(const StructType &type) {
    std::string result;
    result += "{";
    for (const auto &property : type.properties) {
      result += property.first + ": ";
      result += std::visit(*this, *property.second);
      if (property != type.properties.back()) {
        result += ", ";
      }
    }
    result += "}";
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
    return std::make_shared<Type>(
        TypeReference(typeReferenceNode->typeName->name));
  }

  auto interfaceDeclarationNode =
      dynamic_cast<InterfaceDeclarationNode *>(node);
  if (interfaceDeclarationNode) {
    std::vector<NamedType> members;
    for (const auto &member : interfaceDeclarationNode->members) {
      members.push_back(
          std::make_pair(member->name, typeNodeToType(member->type.get())));
    }
    return std::make_shared<Type>(StructType{members});
  }

  auto structTypeNode = dynamic_cast<StructTypeNode *>(node);
  if (structTypeNode) {
    std::vector<NamedType> members;
    for (const auto &member : structTypeNode->members) {
      members.push_back(
          std::make_pair(member->name, typeNodeToType(member->type.get())));
    }
    return std::make_shared<Type>(StructType{members});
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

int findIndex(StructType &structType, std::string propertyName) {
  for (int index = 0; const auto &property : structType.properties) {
    if (property.first == propertyName) {
      return index;
    }
    index += 1;
  }
  return -1;
}