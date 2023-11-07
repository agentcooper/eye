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
class StructType;
enum class PrimitiveType { i64Type, f64Type, voidType, unknownType };
using Type = std::variant<PrimitiveType, FunctionType, StructType>;

class FunctionType {
public:
  std::shared_ptr<Type> returnType;
  std::vector<std::shared_ptr<Type>> parameters;

  FunctionType(std::shared_ptr<Type> returnType,
               std::vector<std::shared_ptr<Type>> parameters)
      : returnType(std::move(returnType)), parameters(std::move(parameters)) {}
};

// TODO: figure out a way to use struct here instead
using NamedType = std::pair<std::string, std::shared_ptr<Type>>;

class StructType {
public:
  std::vector<NamedType> properties;

  StructType(
      std::vector<std::pair<std::string, std::shared_ptr<Type>>> properties)
      : properties(std::move(properties)) {}
};

std::string typeToString(Type &type);

std::shared_ptr<Type> typeNodeToType(Node *node);