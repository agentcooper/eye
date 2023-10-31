function makeCounter(initialValue) {
  let counter = initialValue;

  function inc() {
    counter += 1;
    return counter;
  }

  function reset() {
    counter = 0;
  }

  return { inc, reset };
}

let { inc, reset } = makeCounter(10);
console.log(inc()); // 11
console.log(inc()); // 12
reset();
console.log(inc()); // 1
