#pragma once

#include <string>

struct Token {
public:
  enum class Kind {
    LessThan,
    GreaterThan,
    Integer,
    FloatingPoint,
    Plus,
    Minus,
    LeftParen,
    RightParen,
    LeftCurly,
    RightCurly,
    Semicolon,
    Colon,
    Comma,
    Dot,
    If,
    Else,
    Asterisk,
    Slash,
    Percent,
    Identifier,
    Function,
    Interface,
    Equals,
    DoubleEquals,
    EqualsGreaterThan,
    AmpersandAmpersand,
    BarBar,
    Return,
    Let,
    String,
    Declare,
    For,
    True,
    False,
    End,
    Unexpected,
  };

  Token(Kind kind, const char *beg, std::size_t len) noexcept
      : m_kind{kind}, m_lexeme(beg, len) {}

  Token(Kind kind, const char *beg, const char *end) noexcept
      : m_kind{kind}, m_lexeme(beg, std::distance(beg, end)) {}

  std::string_view lexeme() const noexcept { return m_lexeme; }
  Kind kind() const noexcept { return m_kind; }

  bool is_valid() const {
    return m_kind != Token::Kind::End && m_kind != Token::Kind::Unexpected;
  }

private:
  Kind m_kind{};
  std::string_view m_lexeme{};
};

const char *kindToString(const Token::Kind kind);