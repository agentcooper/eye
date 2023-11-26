#pragma once

#include <iostream>

constexpr auto enableRuntimeDebugPrint = false;

template <typename... T> void debug(const T &...t) {
  if (!enableRuntimeDebugPrint) {
    return;
  }
  std::cout << "[Runtime] ";
  (void)std::initializer_list<int>{(std::cout << t << "", 0)...};
  std::cout << std::endl;
}