#include <stdio.h>
#include <stdlib.h>

typedef uintptr_t (*CallFunction)(void *);

struct Function {
  CallFunction call;
  void *env;
};

struct Env {
  int counter;
};

int inc_block(void *env) {
  struct Env *_env = ((struct Env *)env);
  _env->counter += 1;
  return _env->counter;
}

void reset_block(void *env) {
  struct Env *_env = ((struct Env *)env);
  _env->counter = 0;
}

struct Result {
  struct Function *inc;
  struct Function *reset;
};

struct Result makeCounter(int initialValue) {
  int counter = initialValue;

  struct Env *env = (struct Env *)malloc(sizeof(struct Env));
  env->counter = counter;

  struct Function *inc_closure =
      (struct Function *)malloc(sizeof(struct Function));
  inc_closure->env = env;
  inc_closure->call = (CallFunction)&inc_block;

  struct Function *reset_closure =
      (struct Function *)malloc(sizeof(struct Function));
  reset_closure->env = env;
  reset_closure->call = (CallFunction)&reset_block;

  return (struct Result){.inc = inc_closure, .reset = reset_closure};
}

int main() {
  struct Result result = makeCounter(10);

  struct Function *inc = result.inc;
  struct Function *reset = result.reset;

  printf("%d\n", (int)inc->call(inc->env)); // 11
  printf("%d\n", (int)inc->call(inc->env)); // 12
  reset->call(reset->env);
  printf("%d\n", (int)inc->call(inc->env)); // 1
}
