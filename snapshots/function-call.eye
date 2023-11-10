function bar(x: i64): i64 {
  return x + 1;
}

function foo(a: i64, b: i64): i64 {
  return bar(a) + bar(b);
}

function main(): i64 {
  print(foo(10, 20));
  return 0;
}