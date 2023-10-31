#pragma once

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility> // std::move
#include <variant>
#include <vector>

#include "Node.hpp"

class FunctionType;
enum class PrimitiveType { i64Type, voidType, unknownType };
using Type = std::variant<PrimitiveType, FunctionType>;

class FunctionType {
public:
  std::shared_ptr<Type> returnType;
  std::vector<std::shared_ptr<Type>> parameters;

  FunctionType(std::shared_ptr<Type> returnType,
               std::vector<std::shared_ptr<Type>> parameters)
      : returnType(std::move(returnType)), parameters(std::move(parameters)) {}
};

std::string typeToString(Type &type);

std::shared_ptr<Type> typeNodeToType(Node *node);