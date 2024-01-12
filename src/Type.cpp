#include "Type.hpp"

struct TypePrinterVisitor {
  std::string operator()(const PrimitiveType &type) {
    switch (type) {
    case PrimitiveType::i64Type:
      return "i64";
    case PrimitiveType::f64Type:
      return "f64";
    case PrimitiveType::booleanType:
      return "boolean";
    case PrimitiveType::charType:
      return "char";
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
  std::string operator()(const PointerType &type) {
    std::string result;
    result += "Pointer<";
    result += std::visit(*this, *type.type);
    result += ">";
    return result;
  }
  std::string operator()(const ArrayType &type) {
    std::string result;
    result += "Array<";
    result += std::visit(*this, *type.type);
    result += ", " + std::to_string(type.size);
    result += ">";
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

bool isPrimitiveType(Type &type, PrimitiveType primitiveType) {
  PrimitiveType *t = std::get_if<PrimitiveType>(&type);
  if (!t) {
    return false;
  }
  return *t == primitiveType;
}

std::shared_ptr<Type> typeNodeToType(Node *node) {
  auto typeReferenceNode = dynamic_cast<TypeReferenceNode *>(node);
  if (typeReferenceNode) {
    auto *identifier =
        dynamic_cast<IdentifierNode *>(typeReferenceNode->typeName.get());
    assert(identifier);

    if (identifier->name == "i64") {
      return std::make_shared<Type>(PrimitiveType::i64Type);
    }
    if (identifier->name == "f64") {
      return std::make_shared<Type>(PrimitiveType::f64Type);
    }
    if (identifier->name == "boolean") {
      return std::make_shared<Type>(PrimitiveType::booleanType);
    }
    if (identifier->name == "char") {
      return std::make_shared<Type>(PrimitiveType::charType);
    }
    if (identifier->name == "string") {
      return std::make_shared<Type>(PrimitiveType::stringType);
    }
    if (identifier->name == "void") {
      return std::make_shared<Type>(PrimitiveType::voidType);
    }
    if (identifier->name == "Pointer") {
      auto t = typeNodeToType(typeReferenceNode->typeParameters[0].get());
      return std::make_shared<Type>(PointerType(std::move(t)));
    }
    if (identifier->name == "Array") {
      auto t = typeNodeToType(typeReferenceNode->typeParameters[0].get());

      auto literalNode = dynamic_cast<LiteralTypeNode *>(
          typeReferenceNode->typeParameters[1].get());
      if (!literalNode) {
        throw std::runtime_error("Expected literal node");
      }

      auto numericLiteralNode =
          dynamic_cast<NumericLiteralNode *>(literalNode->literal.get());
      if (!numericLiteralNode) {
        throw std::runtime_error("Expected numeric literal node");
      }

      return std::make_shared<Type>(
          ArrayType(std::move(t), (int)numericLiteralNode->value));
    }
    return std::make_shared<Type>(TypeReference(identifier->name));
  }

  auto interfaceDeclarationNode =
      dynamic_cast<InterfaceDeclarationNode *>(node);
  if (interfaceDeclarationNode) {
    std::vector<NamedType> members;
    for (const auto &member : interfaceDeclarationNode->members) {
      auto *propertySignature =
          dynamic_cast<PropertySignatureNode *>(member.get());
      assert(propertySignature);
      members.push_back(
          std::make_pair(propertySignature->name,
                         typeNodeToType(propertySignature->type.get())));
    }
    return std::make_shared<Type>(StructType{members});
  }

  auto structTypeNode = dynamic_cast<StructTypeNode *>(node);
  if (structTypeNode) {
    std::vector<NamedType> members;
    for (const auto &member : structTypeNode->members) {
      auto *propertySignature =
          dynamic_cast<PropertySignatureNode *>(member.get());
      assert(propertySignature);
      members.push_back(
          std::make_pair(propertySignature->name,
                         typeNodeToType(propertySignature->type.get())));
    }
    return std::make_shared<Type>(StructType{members});
  }

  auto functionTypeNode = dynamic_cast<FunctionTypeNode *>(node);
  if (functionTypeNode) {
    std::vector<std::shared_ptr<Type>> parameters;
    for (const auto &parameterNode : functionTypeNode->parameters) {
      auto *parameter = dynamic_cast<ParameterNode *>(parameterNode.get());
      parameters.push_back(typeNodeToType(parameter->type.get()));
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