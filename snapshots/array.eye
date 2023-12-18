function main(): i64 {
  let arr: Array<string, 3>;

  arr[0] = "aaa";
  arr[1] = "bbb";
  arr[2] = "ccc";

  for (let i = 0; i < 3; i += 1) {
    print(arr[i]);
  }

  return 0;
}