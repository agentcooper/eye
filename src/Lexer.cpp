#include <cctype>
#include <iomanip>
#include <iostream>
#include <memory>

#include "Lexer.hpp"

bool is_space(char c) noexcept {
  return std::isspace(static_cast<unsigned char>(c));
}

bool is_digit(char c) noexcept {
  return std::isdigit(static_cast<unsigned char>(c));
}

bool is_alpha(char c) noexcept {
  return std::isalpha(static_cast<unsigned char>(c));
}

bool is_identifier_char(char c) noexcept {
  if (is_digit(c) || is_alpha(c)) {
    return true;
  }
  switch (c) {
  case '_':
    return true;
  default:
    return false;
  }
}

Token Lexer::getNextToken() noexcept {
  while (is_space(peek())) {
    get();
  }
  auto c = peek();
  if (is_digit(c)) {
    return parseNumber();
  }
  if (is_identifier_char(c)) {
    auto token = parseIdentifier();
    auto lexeme = token.lexeme();
    if (lexeme == "return") {
      return fromLexeme(Token::Kind::Return, lexeme);
    }
    if (lexeme == "function") {
      return fromLexeme(Token::Kind::Function, lexeme);
    }
    if (lexeme == "if") {
      return fromLexeme(Token::Kind::If, lexeme);
    }
    if (lexeme == "else") {
      return fromLexeme(Token::Kind::Else, lexeme);
    }
    if (lexeme == "let") {
      return fromLexeme(Token::Kind::Let, lexeme);
    }
    return token;
  }
  switch (c) {
  case '\0':
    return Token(Token::Kind::End, m_source, 1);
  default:
    return single(Token::Kind::Unexpected);
  case '(':
    return single(Token::Kind::LeftParen);
  case ')':
    return single(Token::Kind::RightParen);
  case '<':
    return single(Token::Kind::LessThan);
  case '>':
    return single(Token::Kind::GreaterThan);
  case '{':
    return single(Token::Kind::LeftCurly);
  case '}':
    return single(Token::Kind::RightCurly);
  case '+':
    return single(Token::Kind::Plus);
  case '-':
    return single(Token::Kind::Minus);
  case ',':
    return single(Token::Kind::Comma);
  case ';':
    return single(Token::Kind::Semicolon);
  case ':':
    return single(Token::Kind::Colon);
  case '=':
    if (*(m_source + 1) == '=') {
      auto token = Token(Token::Kind::DoubleEquals, m_source, 2);
      m_source += 2;
      return token;
    }
    if (*(m_source + 1) == '>') {
      auto token = Token(Token::Kind::EqualsGreaterThan, m_source, 2);
      m_source += 2;
      return token;
    }
    return single(Token::Kind::Equals);
  case '*':
    return single(Token::Kind::Asterisk);
  }
}

Token Lexer::parseIdentifier() noexcept {
  const char *start = m_source;
  get();
  while (is_identifier_char(peek()))
    get();
  return Token(Token::Kind::Identifier, start, m_source);
}

Token Lexer::parseNumber() noexcept {
  const char *start = m_source;
  get();
  while (is_digit(peek()))
    get();
  return Token(Token::Kind::Number, start, m_source);
}

void Lexer::printDebugOutput() {
  for (auto token = getNextToken(); token.is_valid(); token = getNextToken()) {
    std::cout << std::setw(18) << kindToString(token.kind()) << " |"
              << token.lexeme() << "|" << std::endl;
  }
}
