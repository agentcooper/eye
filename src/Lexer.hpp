#pragma once

#include <string>

#include "Token.hpp"

class Lexer {
public:
  Lexer(const std::string &source) : m_source(source.c_str()) {}
  Token getNextToken() noexcept;

  void printDebugOutput();

private:
  const char *m_source = nullptr;
  char peek() const noexcept { return *m_source; }
  char get() noexcept { return *m_source++; }
  Token single(const Token::Kind kind) noexcept {
    return Token(kind, m_source++, 1);
  }
  Token fromLexeme(const Token::Kind kind,
                   const std::string_view lexeme) noexcept {
    return Token(kind, lexeme.begin(), lexeme.size());
  }

  Token parseIdentifier() noexcept;
  Token parseChar() noexcept;
  Token parseString() noexcept;
  Token parseNumber() noexcept;
};

void printLexerOutput(Lexer &lexer);