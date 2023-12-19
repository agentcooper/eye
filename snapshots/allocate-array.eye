function main(): i64 {
  let width = 5;
  let height = 10;

  let map: Pointer<Pointer<string>> = allocate(8 * height);

  for (let y = 0; y < height; y += 1) {
    map[y] = allocate(8 * width);
    for (let x = 0; x < width; x += 1) {
      map[y][x] = "(" + x + "," + y + ")";
    }
  }

  for (let y = 0; y < height; y += 1) {
    for (let x = 0; x < width; x += 1) {
      print(map[y][x]);
    }
  }

  return 0;
}