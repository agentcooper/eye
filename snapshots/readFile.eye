function test(): i64 {
  let input = readFile("input/hello.txt");
  print(input);
  return 0;
}

function main(): i64 {
  return test();
}