function minus(a: f64, b: f64): f64 {
  return a - b;
}

function apply(n: f64, f: (x: f64) => f64): f64 {
  return f(n);
}

function main(): i64 {
  let a = 5.0;
  let b = 1.5;
  print(a + b);
  print(a - b);
  let result = apply(3.14, (x: f64): f64 => {
    return x + 3.14;
  });
  print(result);
  return 0;
}