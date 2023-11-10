interface Counter {
  inc: () => i64,
  reset: () => void
}

function makeCounter(initialValue: i64): Counter {
  let counter = initialValue;
  let inc = (): i64 => {
    counter = counter + 1;
    return counter;
  };
  let reset = (): void => {
    counter = 0;
  };
  return { inc: inc, reset: reset };
}

function main(): i64 {
  let counter = makeCounter(10);
  let inc = counter.inc;
  let reset = counter.reset;
  print(inc());
  print(inc());
  reset();
  print(inc());
  print(inc());
  return 0;
}