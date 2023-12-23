// rustc -o out_rs program.rs && ./out_rs

struct Counter {
  inc: Box<dyn FnMut(u32) -> u32>,
  reset: Box<dyn FnMut() -> ()>
}

fn make_counter(initial: u32) -> Counter {
  let mut counter = initial;

  let inc = move |by: u32| -> u32 {
    counter += by;
    return counter;
  };

  let reset = move || -> () {
    counter = initial;
  };

  return Counter { inc: Box::new(inc), reset: Box::new(reset) };
}

fn main() {
  let mut counter = make_counter(0);
  println!("{}", (counter.inc)(42)); // 42
  println!("{}", (counter.inc)(42)); // 84
  (counter.reset)();
  println!("{}", (counter.inc)(1)); // 85
}