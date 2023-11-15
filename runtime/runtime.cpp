#include <cstdio>

extern "C" {
void printI64(void *env, int n) { printf("%d\n", n); }

void printF64(void *env, double n) { printf("%g\n", n); }

void printString(void *env, char *s) { printf("%s\n", s); }

void *allocate(int bytes) {
  // printf("Allocating %d bytes...\n", bytes);
  void *data = ::operator new(bytes);
  return data;
}
}
