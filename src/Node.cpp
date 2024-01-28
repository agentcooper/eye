#include "Node.hpp"

std::unique_ptr<Node> cloneNode(const std::unique_ptr<Node> &node) {
  return node ? node->clone() : nullptr;
}

std::vector<std::unique_ptr<Node>>
cloneNodes(const std::vector<std::unique_ptr<Node>> &nodes) {
  std::vector<std::unique_ptr<Node>> copy;
  copy.reserve(nodes.size());
  for (const auto &node : nodes) {
    copy.push_back(node->clone());
  }
  return copy;
}