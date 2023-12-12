// deno run prototype.ts

class List {
  value: number;
  next: List | null;
}

function newList(value: number, next: List | null) {
  const list = new List();
  list.value = value;
  list.next = next;
  return list;
}

function printList(list: List) {
  let s = "[";
  for (let current: List | null = list; current != null; current = current.next) {
    s += current.value;
    if (current.next != null) {
      s += ", ";
    }
  }
  s += "]";
  console.log(s);
}

let list = newList(1, newList(2, newList(3, newList(4, newList(5, null)))));
printList(list);