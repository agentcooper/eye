function isDigit(c: char): boolean {
  return c >= '0' && c <= '9'; 
}

function not(b: boolean): boolean {
  if (b) {
    return false;
  }
  return true;
}

function main(): i64 {
  let input = readFile("input/advent-of-code-2023-1.txt");
  let sum = 0;
  
  let firstDigit = 0;
  let lastDigit = 0;
  let foundFirstDigit = false;

  let inputLength = string_length(input);

  for (let i = 0; i < inputLength; i = i + 1) {
    let c = input[i];

    if (isDigit(c)) {
      let digit = char_to_i64(c);

      if (not(foundFirstDigit)) {
        firstDigit = digit;
        foundFirstDigit = true;
      }
      lastDigit = digit;
    }

    if (c == '\n' || i == inputLength - 1) {
      sum = sum + firstDigit * 10 + lastDigit;
      foundFirstDigit = false;
    }
  }

  print(sum);
  return 0;
}