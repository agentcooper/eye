function text(): i64 {
  let s = "abc\ndef\nghi";
  print(s);

  let l = string_length(s);
  print("Length: " + l);

  for (let i = 0; i < l; i = i + 1) {
    if (s[i] == '\n') {
      print("Found line break at index " + i);
    }
  }

  return 0;
}

function main(): i64 {
  return text();
}