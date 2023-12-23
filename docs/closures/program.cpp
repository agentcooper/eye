// g++ -std=c++20 -o out_cpp program.cpp && ./out_cpp

#include <functional>
#include <iostream>

struct Counter {
  std::function<int(int)> inc;
  std::function<void()> reset;
};

Counter makeCounter(int initial) {
  auto counter = initial;

  auto inc = [=](int by) mutable -> int {
    counter += by;
    return counter;
  };

  auto reset = [=]() mutable { counter = initial; };

  return (Counter){.inc = inc, .reset = reset};
}

int main() {
  auto counter = makeCounter(0);
  std::cout << counter.inc(42) << std::endl; // 42
  std::cout << counter.inc(42) << std::endl; // 84
  counter.reset();
  std::cout << counter.inc(1) << std::endl; // 85
}