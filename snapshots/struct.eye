function main(): i64 {
  let foo = {
    a: {
      b: {
        c: {
          d: {
            value: 4
          },
          value: 3
        },
        value: 2
      },
      value: 1
    }
  };
  print(foo.a.value);
  print(foo.a.b.value);
  print(foo.a.b.c.value);
  print(foo.a.b.c.d.value);

  foo.a.b.c.d.value = 666;
  print(foo.a.b.c.d.value);
  return 0;
}
