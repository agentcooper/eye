#pragma once

#include <format>
#include <functional>
#include <iostream>
#include <string>

static bool tracing_enabled = false;
static int nesting = 0;
struct Defer {
  std::function<void(void)> f;
  Defer(std::function<void(void)> const &f) : f(f) {}
  ~Defer() { f(); }
};

#if defined(TRACE)
#define TRACE_METHOD                                                           \
  std::cout << std::string(nesting * 2, ' ') << __FUNCTION__ << " ("           \
            << traceContext() << ")" << std::endl;                             \
  nesting += 1;                                                                \
  Defer defer([]() { nesting -= 1; });
#else
#define TRACE_METHOD
#endif