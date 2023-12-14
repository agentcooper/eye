function isDigit(c: char): boolean {
  return c >= '0' && c <= '9'; 
}

function match(s: string, start: i64, query: string): boolean {
  for (let i = 0; i < string_length(query); i += 1) {
    if (s[start + i] != query[i]) {
      return false;
    }
  }
  return true;
}

function parseDigit(s: string, index: i64, parseStringDigits: boolean): i64 {
  if (isDigit(s[index])) {
    return char_to_i64(s[index]);
  }
  if (!parseStringDigits) {
    return -1;
  }
  if (match(s, index, "one")) {
    return 1;
  }
  if (match(s, index, "two")) {
    return 2;
  }
  if (match(s, index, "three")) {
    return 3;
  }
  if (match(s, index, "four")) {
    return 4;
  }
  if (match(s, index, "five")) {
    return 5;
  }
  if (match(s, index, "six")) {
    return 6;
  }
  if (match(s, index, "seven")) {
    return 7;
  }
  if (match(s, index, "eight")) {
    return 8;
  }
  if (match(s, index, "nine")) {
    return 9;
  }
  return -1;
}

function main(): i64 {
  let parseStringDigits = true;
  let input = readFile("input/advent-of-code-2023-1.txt");
  let sum = 0;
  
  let firstDigit = 0;
  let lastDigit = 0;
  let foundFirstDigit = false;

  let inputLength = string_length(input);

  for (let i = 0; i < inputLength; i += 1) {
    let c = input[i];

    let digit = parseDigit(input, i, parseStringDigits);
    if (digit != -1) {
      if (!foundFirstDigit) {
        firstDigit = digit;
        foundFirstDigit = true;
      }
      lastDigit = digit;
    }

    if (c == '\n' || i == inputLength - 1) {
      sum += firstDigit * 10 + lastDigit;
      foundFirstDigit = false;
    }
  }

  print(sum);
  return 0;
}