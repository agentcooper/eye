#include <cctype>
#include <iomanip>
#include <iostream>
#include <memory>

#include "Lexer.hpp"

bool isSpace(char c) noexcept {
  return std::isspace(static_cast<unsigned char>(c));
}

bool isDigit(char c) noexcept {
  return std::isdigit(static_cast<unsigned char>(c));
}

bool isAlpha(char c) noexcept {
  return std::isalpha(static_cast<unsigned char>(c));
}

bool isIdentifierChar(char c) noexcept {
  if (isDigit(c) || isAlpha(c)) {
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
start:
  while (isSpace(peek())) {
    get();
  }

  // handle comments
  auto c = peek();
  if (c == '/') {
    get();
    c = peek();
    if (c == '/') {
      get();
      while (peek() != '\n') {
        c = get();
      }
      goto start;
    } else if (c == '=') {
      return single(Token::Kind::SlashEquals);
    } else {
      return single(Token::Kind::Slash);
    }
  }

  if (isDigit(c)) {
    return parseNumber();
  }
  if (c == '"') {
    return parseString();
  }
  if (c == '\'') {
    return parseChar();
  }
  if (isIdentifierChar(c)) {
    auto token = parseIdentifier();
    auto lexeme = token.lexeme();
    if (lexeme == "return") {
      return fromLexeme(Token::Kind::Return, lexeme);
    }
    if (lexeme == "function") {
      return fromLexeme(Token::Kind::Function, lexeme);
    }
    if (lexeme == "interface") {
      return fromLexeme(Token::Kind::Interface, lexeme);
    }
    if (lexeme == "if") {
      return fromLexeme(Token::Kind::If, lexeme);
    }
    if (lexeme == "else") {
      return fromLexeme(Token::Kind::Else, lexeme);
    }
    if (lexeme == "for") {
      return fromLexeme(Token::Kind::For, lexeme);
    }
    if (lexeme == "true") {
      return fromLexeme(Token::Kind::True, lexeme);
    }
    if (lexeme == "false") {
      return fromLexeme(Token::Kind::False, lexeme);
    }
    if (lexeme == "let") {
      return fromLexeme(Token::Kind::Let, lexeme);
    }
    if (lexeme == "declare") {
      return fromLexeme(Token::Kind::Declare, lexeme);
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
  case '[':
    return single(Token::Kind::LeftSquareBracket);
  case ']':
    return single(Token::Kind::RightSquareBracket);
  case '<':
    if (*(m_source + 1) == '=') {
      auto token = Token(Token::Kind::LessThanEquals, m_source, 2);
      m_source += 2;
      return token;
    }
    return single(Token::Kind::LessThan);
  case '>':
    if (*(m_source + 1) == '=') {
      auto token = Token(Token::Kind::GreaterThanEquals, m_source, 2);
      m_source += 2;
      return token;
    }
    return single(Token::Kind::GreaterThan);
  case '!':
    if (*(m_source + 1) == '=') {
      auto token = Token(Token::Kind::ExclamationMarkEquals, m_source, 2);
      m_source += 2;
      return token;
    }
    return single(Token::Kind::ExclamationMark);
  case '{':
    return single(Token::Kind::LeftCurly);
  case '}':
    return single(Token::Kind::RightCurly);
  case '+':
    if (*(m_source + 1) == '=') {
      auto token = Token(Token::Kind::PlusEquals, m_source, 2);
      m_source += 2;
      return token;
    }
    return single(Token::Kind::Plus);
  case '-':
    if (*(m_source + 1) == '=') {
      auto token = Token(Token::Kind::MinusEquals, m_source, 2);
      m_source += 2;
      return token;
    }
    return single(Token::Kind::Minus);
  case ',':
    return single(Token::Kind::Comma);
  case '.':
    return single(Token::Kind::Dot);
  case ';':
    return single(Token::Kind::Semicolon);
  case ':':
    return single(Token::Kind::Colon);
  case '&':
    if (*(m_source + 1) == '&') {
      auto token = Token(Token::Kind::AmpersandAmpersand, m_source, 2);
      m_source += 2;
      return token;
    }
    return single(Token::Kind::Unexpected);
  case '|':
    if (*(m_source + 1) == '|') {
      auto token = Token(Token::Kind::BarBar, m_source, 2);
      m_source += 2;
      return token;
    }
    return single(Token::Kind::Unexpected);
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
    if (*(m_source + 1) == '=') {
      auto token = Token(Token::Kind::AsteriskEquals, m_source, 2);
      m_source += 2;
      return token;
    }
    return single(Token::Kind::Asterisk);
  case '%':
    return single(Token::Kind::Percent);
  }
}

Token Lexer::parseChar() noexcept {
  const char *start = m_source;
  get();
  while (peek() != '\'')
    get();
  get();
  return Token(Token::Kind::Char, start, m_source);
}

Token Lexer::parseString() noexcept {
  const char *start = m_source;
  get();
  while (peek() != '"')
    get();
  get();
  return Token(Token::Kind::String, start, m_source);
}

Token Lexer::parseIdentifier() noexcept {
  const char *start = m_source;
  get();
  while (isIdentifierChar(peek()))
    get();
  return Token(Token::Kind::Identifier, start, m_source);
}

Token Lexer::parseNumber() noexcept {
  const char *start = m_source;
  bool hasFloatingPoint = false;
  get();
  while (auto c = peek()) {
    if (isDigit(c) || c == '.') {
      if (c == '.') {
        hasFloatingPoint = true;
      }
      get();
    } else {
      break;
    }
  }
  if (hasFloatingPoint) {
    return Token(Token::Kind::FloatingPoint, start, m_source);
  }
  return Token(Token::Kind::Integer, start, m_source);
}

void Lexer::printDebugOutput() {
  for (auto token = getNextToken(); token.kind() != Token::Kind::End;
       token = getNextToken()) {
    std::cout << std::setw(18) << kindToString(token.kind()) << " |"
              << token.lexeme() << "|" << std::endl;
    if (token.kind() == Token::Kind::Unexpected) {
      std::cout << "Found an unexpected token!" << std::endl;
      break;
    }
  }
}
