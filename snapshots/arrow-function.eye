function main(): i64 {
  let f0 = (): i64 => { return 666; };

  let f1 = (a: i64): i64 => { return 666 + a; };
  
  let f2 = (a: i64, b: i64): i64 => { return 666 + a + b; };

  let f3 = (a: i64, b: i64, c: i64): i64 => { return 666 + a + b + c; };
  
  print(f0());
  print(f1(1));
  print(f2(1, 2));
  print(f3(1, 2, 3));

  return 0;
}