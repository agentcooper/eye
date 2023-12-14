function cleanUpRegistersForGC(a: i64, b: i64, c: i64, d: i64, e: i64): i64 {
  return a + b + c + d + e;
}

function main(): i64 {
  let s1 = "Hello,";
  let s2 = " World";

  s2 += "!";

  print(s1 + s2);

  let _ = cleanUpRegistersForGC(1, 2, 3, 4, 5);
  return 0;
}