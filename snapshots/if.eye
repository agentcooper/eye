function main(): i64 {
  if (1 == 1) {
    print("OK");
  } else {
    print("FAIL");
  }

  if (1 == 2) {
    print("FAIL");
  } else {
    print("OK");
  }

  if (true) {
    print(0);
    if (true) {
      print(1);
    }
    print(2);
  }
  print(3);

  return 0;
}