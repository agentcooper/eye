interface Size {
  width: i64,
  height: i64
}

interface Coord {
  x: i64,
  y: i64
}

function get_neighbours(coord: Coord, size: Size, callback: (coord: Coord) => void): i64 {
  for (let x = coord.x - 1; x <= coord.x + 1; x += 1) {
    for (let y = coord.y - 1; y <= coord.y + 1; y += 1) {
      if (!(x == coord.x && y == coord.y)) {
        let n: Coord = { x: x, y: y };
        if (x >= 0 && x < size.width && y >= 0 && y < size.height) {
          callback(n);
        }
      }
    }
  }
  return 0;
}

function main(): i64 {
  get_neighbours({ x: 1, y: 0 }, { width: 100, height: 100 }, (n: Coord): void => {
    print("" + n.x + " " + n.y);
  });

  return 0;
}