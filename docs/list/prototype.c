// clang -o c_list prototype.c

#include <stdio.h>
#include <stdlib.h>

struct List {
  int value;
  struct List *next;
};

struct List *newList(int value, struct List *next) {
  struct List *list = (struct List *)malloc(sizeof(struct List));
  list->value = value;
  list->next = next;
  return list;
}

void printList(struct List *list) {
  printf("[");
  for (struct List *current = list; current != NULL; current = current->next) {
    printf("%d", current->value);
    if (current->next != NULL) {
      printf(", ");
    }
  }
  printf("]\n");
}

int main() {
  struct List *list =
      newList(1, newList(2, newList(3, newList(4, newList(5, NULL)))));
  printList(list);
  return 0;
}