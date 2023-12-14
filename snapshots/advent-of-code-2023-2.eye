function is_digit(c: char): boolean {
  return c >= '0' && c <= '9'; 
}

function max(a: i64, b: i64): i64 {
  if (a > b) {
    return a;
  }
  return b;
}

function main(): i64 {
  let input = readFile("input/advent-of-code-2023-2.txt");
  let input_length = string_length(input);

  let answer1 = 0;
  let answer2 = 0;

  let max_red = 12;
  let max_green = 13;
  let max_blue = 14;

  let game_id = 0;
  let is_possible = true;
  let number = 0;
  let red = 0;
  let green = 0;
  let blue = 0;

  let lastChar = '';

  // for part 2
  let game_max_red = 0;
  let game_max_green = 0;
  let game_max_blue = 0;

  for (let i = 0; i < input_length; i = i + 1) {
    let c = input[i];

    if (is_digit(c)) {
      number = number * 10 + char_to_i64(c);
    }

    if (c == ':') {
      game_id = number;
      number = 0;
    }

    if (c == ',' || c == ';' || c == '\n') {
      if (lastChar == 'd') {
        red = red + number;
        game_max_red = max(red, game_max_red);
      }
      if (lastChar == 'e') {
        blue = blue + number;
        game_max_blue = max(blue, game_max_blue);
      }
      if (lastChar == 'n') {
        green = green + number;
        game_max_green = max(green, game_max_green);
      }
      number = 0;
    }

    if (c == ';' || c == '\n') {
      if (red > max_red || green > max_green || blue > max_blue) {
        is_possible = false;
      }

      red = 0;
      green = 0;
      blue = 0;
    }

    if (c == '\n' || i == input_length - 1) {
      if (is_possible) {
        answer1 = answer1 + game_id;
      }
      answer2 = answer2 + game_max_red * game_max_green * game_max_blue;

      red = 0;
      green = 0;
      blue = 0;

      game_max_red = 0;
      game_max_green = 0;
      game_max_blue = 0;

      number = 0;
      game_id = 0;
      is_possible = true;
    }

    lastChar = c;
  }

  print(answer1);
  print(answer2);

  return 0;
}
