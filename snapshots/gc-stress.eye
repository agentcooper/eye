function printBigString(): void {
  let s = "All work and no play makes Jack a dull boy. ";
  let bigString = s + s + s + s + s + s + s + s + s + s + s + s + s + s + s + s + s + s + s + s;
  print(bigString);
}

function main(): i64 {
  printBigString();
  printBigString();
  printBigString();
  return 0;
}