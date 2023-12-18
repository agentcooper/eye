function main(): i64 {
  let map: Array<Array<string, 5>, 5>;

  for (let y = 0; y < 5; y += 1) {
    for (let x = 0; x < 5; x += 1) {
      map[y][x] = "(" + x + "," + y + ")";
      print(map[y][x], ' ');
    }
    print("");
  }

  return 0;
}