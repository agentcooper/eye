interface List {
  value: i64,
  next: Pointer<List>
}

function pair(v: i64, n: Pointer<List>): Pointer<List> {
  let p: Pointer<List> = allocate(sizeof(List));
  (*p).value = v;
  (*p).next = n;
  return p;
}

function forEach(list: Pointer<List>, callback: (current: i64) => void): void {
  for (let current: Pointer<List> = list; current != null; current = (*current).next) {
    callback((*current).value);
  }
}

function reduce(list: Pointer<List>, start: i64, callback: (acc: i64, current: i64) => i64): i64 {
  let result = start;
  for (let current: Pointer<List> = list; current != null; current = (*current).next) {
    result = callback(result, (*current).value);
  }
  return result;
}

function main(): i64 {
  let list = pair(1, pair(2, pair(3, pair(4, null))));
  
  forEach(list, (n: i64): void => {
    print(n);
  });

  let sum = reduce(list, 0, (acc: i64, n: i64): i64 => {
    return acc + n;
  });
  print("Sum is " + sum);

  return 0;
}