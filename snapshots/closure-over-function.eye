function makeCounter(): () => i64 {
  let counter = 1;

  let plus1 = (x: i64): i64 => {
    return x + 1;
  };

  let inc = (): i64 => {
    counter = plus1(counter);
    return counter;
  };

  return inc;
}

function main(): i64 {
  let x = makeCounter();
  print(x());
  print(x());
  return 0;
}