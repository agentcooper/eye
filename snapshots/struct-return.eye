interface Foo {
  a: i64,
  b: i64
}

function getFoo(): Foo {
  return { a: 111, b: 333 };
}

function getBar(): { c: i64 } {
  return { c: 222 };
}

function main(): i64 {
  let foo = getFoo();
  let bar = getBar();
  print(foo.a + foo.b + bar.c);
  return 0;
}