#include "Token.hpp"

const char *kindToString(const Token::Kind kind) {
  using enum Token::Kind;
  switch (kind) {
  case Number:
    return "Number";
  case Plus:
    return "Plus";
  case Minus:
    return "Minus";
  case LeftParen:
    return "LeftParen";
  case RightParen:
    return "RightParen";
  case LeftCurly:
    return "LeftCurly";
  case RightCurly:
    return "RightCurly";
  case Semicolon:
    return "Semicolon";
  case Colon:
    return "Colon";
  case Comma:
    return "Comma";
  case Asterisk:
    return "Asterisk";
  case Identifier:
    return "Identifier";
  case If:
    return "If";
  case Else:
    return "Else";
  case Equals:
    return "Equals";
  case DoubleEquals:
    return "DoubleEquals";
  case EqualsGreaterThan:
    return "EqualsGreaterThan";
  case LessThan:
    return "LessThan";
  case GreaterThan:
    return "GreaterThan";
  case Return:
    return "Return";
  case Function:
    return "Function";
  case Let:
    return "Let";
  case End:
    return "End";
  case Unexpected:
    return "Unexpected";
  }
}