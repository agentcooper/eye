function plus1(n: i64): i64 {
  return n + 1;
}

function minus1(n: i64): i64 {
  return n - 1;
}

function foo(n: i64, f: (n: i64) => i64): i64 {
  return f(n);
}

function main(): i64 {
  print(foo(100, plus1));
  print(foo(100, minus1));
  return 0;
}