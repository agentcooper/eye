function main(): i64 {
  let a = true;
  let b = false;
  print(a);
  print(b);

  print("And:");

  print(false && false);
  print(false && true);
  print(true && false);
  print(true && true);

  print("Or:");
  
  print(false || false);
  print(false || true);
  print(true || false);
  print(true || true);

  if (true) {
    print("true!");
  }

  if (false) {
    print("oh no!");
  }

  return 0;
}