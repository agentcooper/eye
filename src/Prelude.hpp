#pragma once

#include <string>

std::string getPrelude() {
  std::string result;
  result += "declare function print_i64(n: i64): void;\n";
  result += "declare function print_f64(n: f64): void;\n";
  result += "declare function print_boolean(b: boolean): void;\n";
  result += "declare function print_char(c: char): void;\n";
  result += "declare function print_string(s: string): void;\n";

  result += "declare function joinStrings(s1: string, s2: string): string;\n";
  result += "declare function string_length(s: string): i64;\n";

  result += "declare function i64_to_string(n: i64): string;\n";
  result += "declare function char_to_i64(c: char): i64;\n";

  result += "declare function readFile(filePath: string): string;\n";

  result += "declare function allocate(size: i64): Pointer<void>;\n";
  return result;
}