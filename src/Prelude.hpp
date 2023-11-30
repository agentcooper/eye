#pragma once

#include <string>

std::string getPrelude() {
  std::string result;
  result += "declare function print_i64(n: i64): void;\n";
  result += "declare function print_f64(n: f64): void;\n";
  result += "declare function print_string(s: string): void;\n";

  result += "declare function joinStrings(s1: string, s2: string): string;\n";
  return result;
}