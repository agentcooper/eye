interface Foo {
  a: i64,
  b: i64
}

function main(): i64 {
  let foo: Foo = { a: 1, b: 2};
  print(foo.a + foo.b);
  return 0;
}