interface Color {
  r: i64,
  g: i64,
  b: i64
}

function main(): i64 {
  let n: Pointer<i64> = allocate(sizeof(i64));
  *n = 666;
  print(*n);

  let s: Pointer<Color> = allocate(sizeof(Color));
  (*s).r = 255;
  (*s).g = 255;
  (*s).b = 255;

  print((*s).r);
  print((*s).g);
  print((*s).b);

  return 0;
}