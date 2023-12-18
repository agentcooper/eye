#include <cstdio>

#include <stdlib.h>
#include <string.h>

#include "GarbageCollector.hpp"
#include "debug.hpp"

extern "C" {
void beforeStart() { GarbageCollector::init(); }

void __cleanStack(int depth) {
  volatile int a __attribute__((unused)) = 0;
  volatile int b __attribute__((unused)) = 0;
  volatile int c __attribute__((unused)) = 0;
  volatile int d __attribute__((unused)) = 0;
  volatile int e __attribute__((unused)) = 0;
  if (depth > 0) {
    __cleanStack(depth - 1);
  }
}

void __cleanup() {
  // so we can test for memory leaks at the end
  __cleanStack(10);

  // clean registers
  __asm__("mov x0, #0");
  __asm__("mov x1, #0");
  __asm__("mov x2, #0");
  __asm__("mov x3, #0");
  __asm__("mov x4, #0");
  __asm__("mov x5, #0");
  __asm__("mov x6, #0");
  __asm__("mov x7, #0");
  __asm__("mov x8, #0");
  __asm__("mov x9, #0");
  __asm__("mov x10, #0");
  __asm__("mov x11, #0");
  __asm__("mov x12, #0");
  __asm__("mov x13, #0");
  __asm__("mov x14, #0");
  __asm__("mov x15, #0");
  __asm__("mov x16, #0");
  __asm__("mov x17, #0");
  __asm__("mov x18, #0");
  __asm__("mov x19, #0");
  __asm__("mov x20, #0");
  __asm__("mov x21, #0");
  __asm__("mov x22, #0");
  __asm__("mov x23, #0");
  __asm__("mov x24, #0");
  __asm__("mov x25, #0");
  __asm__("mov x26, #0");
  __asm__("mov x27, #0");
  __asm__("mov x28, #0");
}

void beforeExit() {
  __cleanup();
  GarbageCollector::run();
}

void print_i64(const void *env, const int n) { printf("%d\n", n); }

void print_f64(const void *env, const double n) { printf("%g\n", n); }

void print_boolean(const void *env, const bool b) {
  printf("%s\n", b ? "true" : "false");
}

void print_char(const void *env, const char c) { printf("%c\n", c); }

void print_string(const void *env, const char *s, const char terminator) {
  printf("%s\n", s);
}

void print_string_char(const void *env, const char *s, const char terminator) {
  printf("%s%c", s, terminator);
}

char *joinStrings(const void *env, const char *s1, const char *s2) {
  char *result =
      (char *)GarbageCollector::allocate(strlen(s1) + strlen(s2) + 1);
  strcpy(result, s1);
  strcat(result, s2);
  return result;
}

const char *i64_to_string(const void *env, const int value) {
  static const char *s0 = "0";
  if (value == 0) {
    return s0;
  }
  const int number_of_digits = log10(abs(value)) + 1;
  const int number_of_chars = number_of_digits + 1 + (value < 0 ? 1 : 0);
  char *s = (char *)GarbageCollector::allocate(sizeof(char) * number_of_chars);
  snprintf(s, number_of_chars, "%d", value);
  return s;
}

int char_to_i64(const void *env, const char value) { return value - '0'; }

char *readFile(const void *env, const char *filePath) {
  FILE *f = fopen(filePath, "rb");
  if (!f) {
    printf("[Runtime] Can't open file!\n");
    return nullptr;
  }
  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *string = (char *)GarbageCollector::allocate(fsize + 1);
  fread(string, fsize, 1, f);
  fclose(f);

  string[fsize] = 0;
  return string;
}

size_t string_length(const void *env, const char *s) { return strlen(s); }

void *__allocate(const size_t bytes) {
  void *memory = GarbageCollector::allocate(bytes);
  return memory;
}

void *allocate(const void *env, const size_t bytes) {
  void *memory = GarbageCollector::allocate(bytes);
  return memory;
}
}
