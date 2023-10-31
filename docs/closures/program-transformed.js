/**
 * Closures are lifted to the top level with the additional `env` parameter.
 */

function inc_block(env) {
  env.counter += 1;
  return env.counter;
}

function reset_block(env) {
  env.counter = 0;
}

function makeCounter(initialValue) {
  let counter = initialValue;

  const env = { counter: counter };
  const inc = { env: env, call: inc_block };
  const reset = { env: env, call: reset_block };

  return { inc, reset };
}

let { inc, reset } = makeCounter(10);
console.log(inc.call(inc.env)); // 11
console.log(inc.call(inc.env)); // 12
reset.call(reset.env);
console.log(inc.call(inc.env)); // 1
