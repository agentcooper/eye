function main(): i64 {
  let s = "abc\ndef";
  print(s);

  let l = string_length(s);
  print(l);

  if (s[3] == '\n') {
    print("Newline char at 3");
  }

  return 0;
}