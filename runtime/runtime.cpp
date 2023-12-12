#include <cstdio>

#include <stdlib.h>
#include <string.h>

#include "GarbageCollector.hpp"
#include "debug.hpp"

extern "C" {
void beforeStart() { GarbageCollector::init(); }

void beforeExit() {
  // so we can test for memory leaks at the end
  GarbageCollector::run();
}

void print_i64(const void *env, const int n) { printf("%d\n", n); }

void print_f64(const void *env, const double n) { printf("%g\n", n); }

void print_boolean(const void *env, const bool b) {
  printf("%s\n", b ? "true" : "false");
}

void print_char(const void *env, const char c) { printf("%c\n", c); }

void print_string(const void *env, const char *s) { printf("%s\n", s); }

char *joinStrings(const void *env, const char *s1, const char *s2) {
  char *result =
      (char *)GarbageCollector::allocate(strlen(s1) + strlen(s2) + 1);
  strcpy(result, s1);
  strcat(result, s2);
  return result;
}

char *i64_to_string(const void *env, const int value) {
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
