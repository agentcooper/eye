function plus1(x: i64): i64 {
  return x + 1;
}

function minus1(x: i64): i64 {
  return x - 1;
}

function choose(f: i64): (x: i64) => i64 {
  if (f == 1) {
    return plus1;
  }
  return minus1;
}

function main(): i64 {
  let a = choose(0);
  print(a(10));

  let b = choose(1);
  print(b(10));

  return 0;
}