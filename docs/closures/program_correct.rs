// rustc -o out_rs program_correct.rs && ./out_rs

use std::cell::RefCell;
use std::rc::Rc;

struct Counter {
  inc: Box<dyn FnMut(i32) -> i32>,
  reset: Box<dyn FnMut()>,
}

fn make_counter(initial: i32) -> Counter {
  let counter = Rc::new(RefCell::new(initial));

  let inc = {
    let counter = Rc::clone(&counter);
    Box::new(move |by| {
      let mut counter = counter.borrow_mut();
      *counter += by;
      *counter
    })
  };

  let reset = {
    let counter = Rc::clone(&counter);
    Box::new(move || {
      let mut counter = counter.borrow_mut();
      *counter = initial;
    })
  };

  Counter { inc, reset }
}

fn main() {
  let mut counter = make_counter(0);
  println!("{}", (counter.inc)(42)); // 42
  println!("{}", (counter.inc)(42)); // 84
  (counter.reset)();
  println!("{}", (counter.inc)(1)); // 1
}