function sum<T>(a: T, b: T): T {
  return a + b;
}

function main(): i64 {
  print(sum(11, 22));
  print(sum(11.1, 22.2));
  print(sum("abc", "def"));

  return 0;
}