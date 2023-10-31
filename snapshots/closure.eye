function makeCounter(): () => i64 {
  let counter = 1;
  let inc = (): i64 => {
    counter = counter + 1;
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