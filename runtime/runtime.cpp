#include <cstdio>

extern "C" {
void print(int n) { printf("%d\n", n); }

void *allocate(int bytes) {
  // printf("Allocating %d bytes...\n", bytes);
  void *data = ::operator new(bytes);
  return data;
}
}
