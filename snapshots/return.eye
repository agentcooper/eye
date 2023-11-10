function boo(x: i64): i64 {
  return x + 1;
}

function main(): i64 {
  let a = 1;
  print(a + boo(1));
  return 0;
}