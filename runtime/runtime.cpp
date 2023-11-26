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

void printI64(void *env, int n) { printf("%d\n", n); }

void printF64(void *env, double n) { printf("%g\n", n); }

void printString(void *env, char *s) { printf("%s\n", s); }

char *joinStrings(void *env, const char *s1, const char *s2) {
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
