function twice(callback: () => void): void {
  callback();
  callback();
}

function main(): i64 {
  let n = 666;

  twice((): void => {
    n = n + 1;
  });

  n += 1;
  print(n);
  
  return 0;
}