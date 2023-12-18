#include <stdio.h>
#include <stdlib.h>

struct Person {
  char *name;
  int age;
};

void Person_print(struct Person *person) {
  printf("Person { name = %s, age = %d }\n", person->name, person->age);
}

void allocate_array() {
  // stack
  int arr_stack[3] = {1, 2, 3};
  printf("arr_stack[2] = %d\n", arr_stack[2]);

  // heap
  int *arr_heap = (int *)malloc(sizeof(int) * 3);
  arr_heap[0] = 1;
  arr_heap[1] = 2;
  arr_heap[2] = 3;
  printf("arr_heap[2] = %d\n", arr_heap[2]);
}

void allocate_struct() {
  // stack
  struct Person person_stack = {.name = "Jack", .age = 42};
  Person_print(&person_stack);

  // heap
  struct Person *person_heap = malloc(sizeof(struct Person));
  person_heap->name = "John";
  person_heap->age = 99;
  Person_print(person_heap);
}

int main() {
  allocate_array();

  allocate_struct();

  return 0;
}