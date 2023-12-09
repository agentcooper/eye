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

  if (true == true) {
    print("OK");
  } else {
    print("FAIL");
  }

  if (false == false) {
    print("OK");
  } else {
    print("FAIL");
  }

  if (true != true) {
    print("FAIL");
  }

  if (false != false) {
    print("FAIL");
  }

  return 0;
}