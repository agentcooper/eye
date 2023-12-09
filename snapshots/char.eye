function main(): i64 {
  let s = "Hello, World!";

  print(s[0]);
  print(s[7]);

  let c = 'h';
  print(c);

  let d = '0';
  if (d >= '0' && d <= '9') {
    print("Digit OK");
  }

  if ('a' == 'a') {
    print("== OK");
  }

  if ('a' == 'b') {
    print("== FAIL");
  }

  if ('a' != 'b') {
    print("!= OK");
  }

  if ('a' != 'a') {
    print("!= FAIL");
  }

  return 0;
}