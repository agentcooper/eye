#include <cstdio>

#include <stdlib.h>
#include <string.h>

extern "C" {
void printI64(void *env, int n) { printf("%d\n", n); }

void printF64(void *env, double n) { printf("%g\n", n); }

void printString(void *env, char *s) { printf("%s\n", s); }

char *joinStrings(void *env, const char *s1, const char *s2) {
  char *result = (char *)malloc(strlen(s1) + strlen(s2) + 1);
  strcpy(result, s1);
  strcat(result, s2);
  return result;
}

void *allocate(int bytes) {
  // printf("Allocating %d bytes...\n", bytes);
  void *data = ::operator new(bytes);
  return data;
}
}
