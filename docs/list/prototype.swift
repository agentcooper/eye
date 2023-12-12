// swiftc -o swift_list prototype.swift

class List {
  var value: Int;
  var next: List?;

  init(_ value: Int, _ next: List?) {
    self.value = value;
    self.next = next;
  }
}

func printList(_ list: List) {
  print("[", terminator: "");
  var current: List? = list;
  while (current != nil) {
    print(current!.value, terminator: "");
    if (current!.next != nil) {
      print(", ", terminator: "");
    }
    current = current!.next;
  }
  print("]");
}

func newList(_ value: Int, _ next: List?) -> List {
  return List(value, next);
}

let list = newList(1, newList(2, newList(3, newList(4, newList(5, nil)))));
printList(list);