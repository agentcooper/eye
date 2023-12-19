interface Coord {
  x: i64,
  y: i64
}

function main(): i64 {
  let size = 10;
  let arr: Pointer<Coord> = allocate(sizeof(Coord) * size);

  for (let i = 0; i < size; i += 1) {
    arr[i].x = i;
    arr[i].y = i;
  }

  for (let i = 0; i < size; i += 1) {
    print("(" + arr[i].x + ", " + arr[i].y + ")");
  }

  return 0;
}