// swift program.swift

struct Counter {
  let inc: (Int) -> Int
  let reset: () -> Void
}

func makeCounter(initial: Int) -> Counter {
  var counter = initial

  let inc = { (by: Int) -> Int in
    counter += by
    return counter
  }

  let reset = { () -> Void in
    counter = initial
  }

  return Counter(inc: inc, reset: reset)
}

let counter = makeCounter(initial: 0)
print(counter.inc(42)) // 42
print(counter.inc(42)) // 84
counter.reset()
print(counter.inc(1)) // 1