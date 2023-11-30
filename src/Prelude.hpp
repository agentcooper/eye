#pragma once

#include <string>

std::string getPrelude() {
  std::string result;
  result += "declare function printI64(n: i64): void;\n\n";
  result += "declare function printF64(n: f64): void;\n\n";
  result += "declare function printString(s: string): void;\n\n";
  result += "declare function joinStrings(s1: string, s2: string): string;\n\n";
  return result;
}