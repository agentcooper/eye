// node program.js

function makeCounter(initialValue) {
  let counter = initialValue;

  function inc(by) {
    counter += by;
    return counter;
  }

  function reset() {
    counter = initialValue;
  }

  return { inc, reset };
}

let { inc, reset } = makeCounter(0);
console.log(inc(42)); // 42
console.log(inc(42)); // 84
reset();
console.log(inc(1)); // 1
