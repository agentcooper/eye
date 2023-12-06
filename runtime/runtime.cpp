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

void print_string(const void *env, const char *s) { printf("%s\n", s); }

char *joinStrings(const void *env, const char *s1, const char *s2) {
  char *result =
      (char *)GarbageCollector::allocate(strlen(s1) + strlen(s2) + 1);
  strcpy(result, s1);
  strcat(result, s2);
  return result;
}

void *allocate(const size_t bytes) {
  void *memory = GarbageCollector::allocate(bytes);
  return memory;
}
}
