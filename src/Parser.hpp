#pragma once

#include <iostream>
#include <map>
#include <vector>

#include "Lexer.hpp"
#include "Node.hpp"
#include "Trace.hpp"

class Parser {
  Lexer &lexer;

  Token currentToken;
  Token nextToken;

  // An operation with a higher precedence is performed before operations with
  // lower precedence.
  std::map<Token::Kind, int> precedence;

  // See `./Trace.hpp`
  std::string traceContext() {
    return "token = '" + std::string(currentToken.lexeme()) + "'";
  }

public:
  Parser(Lexer &lexer)
      : lexer(lexer), currentToken(lexer.getNextToken()),
        nextToken(lexer.getNextToken()) {

    // https://en.wikipedia.org/wiki/Order_of_operations#Programming_languages
    precedence[Token::Kind::Equals] = 5;
    precedence[Token::Kind::LessThan] = 10;
    precedence[Token::Kind::GreaterThan] = 10;
    precedence[Token::Kind::DoubleEquals] = 10;
    precedence[Token::Kind::Plus] = 20;
    precedence[Token::Kind::Minus] = 20;
    precedence[Token::Kind::Asterisk] = 40;
  };

  const Token &peekToken() { return nextToken; }

  void getNextToken() {
    currentToken = nextToken;
    nextToken = lexer.getNextToken();
  }

  int getCurrentTokenPrecedence() {
    int value = precedence[currentToken.kind()];
    if (value <= 0)
      return -1;
    return value;
  }

  void expect(const Token::Kind kind) {
    if (currentToken.kind() != kind) {
      std::string message;
      message += "Expect token ";
      message += kindToString(kind);
      message += ", but found ";
      message += kindToString(currentToken.kind());
      throw std::runtime_error(message);
    }
  }

  std::unique_ptr<Node> parseType() {
    TRACE_METHOD;

    if (currentToken.kind() == Token::Kind::Identifier) {
      std::string name{currentToken.lexeme()};
      getNextToken(); // eat identifier
      auto identifierNode = std::make_unique<IdentifierNode>(name);
      auto typeReferenceNode =
          std::make_unique<TypeReferenceNode>(std::move(identifierNode));
      return typeReferenceNode;
    }

    if (currentToken.kind() == Token::Kind::LeftParen) {
      getNextToken(); // eat '('

      std::vector<std::unique_ptr<ParameterNode>> parameters =
          parseParameters();

      getNextToken(); // eat ')'

      getNextToken(); // eat '=>'

      auto returnType = parseType();

      return std::make_unique<FunctionTypeNode>(std::move(parameters),
                                                std::move(returnType));
    }

    throw std::runtime_error("Can't parse type");
  }

  std::unique_ptr<Node> parseArrowFunction() {
    TRACE_METHOD;

    auto parameters = parseParameters();

    getNextToken(); // eat ')'

    getNextToken(); // eat ':'

    auto returnType = parseType();

    expect(Token::Kind::EqualsGreaterThan);
    getNextToken();

    auto block = parseBlock();

    return std::make_unique<ArrowFunctionExpressionNode>(
        std::move(parameters), std::move(returnType), std::move(block));
  }

  std::unique_ptr<Node> parseParenExpression() {
    TRACE_METHOD;

    getNextToken(); // eat '('

    if ((currentToken.kind() == Token::Kind::Identifier &&
         nextToken.kind() == Token::Kind::Colon) ||
        currentToken.kind() == Token::Kind::RightParen) {
      return parseArrowFunction();
    }

    auto V = parseExpression();
    if (!V) {
      throw std::runtime_error("Unexpected");
    }

    expect(Token::Kind::RightParen);
    getNextToken(); // eat ')'

    return V;
  }

  std::unique_ptr<Node> parseBinaryExpression(int expressionPrecedence,
                                              std::unique_ptr<Node> lhs) {
    TRACE_METHOD;

    while (true) {
      int precedence = getCurrentTokenPrecedence();

      if (precedence < expressionPrecedence)
        return lhs;

      Token::Kind BinOp = currentToken.kind();
      getNextToken(); // eat operator

      auto RHS = parsePrimary();
      if (!RHS)
        return nullptr;

      int NextPrec = getCurrentTokenPrecedence();
      if (precedence < NextPrec) {
        RHS = parseBinaryExpression(precedence + 1, std::move(RHS));
        if (!RHS)
          return nullptr;
      }

      lhs = std::make_unique<BinaryExpressionNode>(BinOp, std::move(lhs),
                                                   std::move(RHS));
    }
  }

  std::unique_ptr<Node> parseIdentifierExpression() {
    TRACE_METHOD;

    std::string name{currentToken.lexeme()};
    getNextToken(); // eat identifier

    if (currentToken.kind() != Token::Kind::LeftParen) {
      return std::make_unique<IdentifierNode>(name);
    }

    getNextToken(); // eat '('
    std::vector<std::unique_ptr<Node>> arguments;
    if (currentToken.kind() != Token::Kind::RightParen) {
      while (true) {
        if (auto argument = parseExpression())
          arguments.push_back(std::move(argument));
        else
          return nullptr;

        if (currentToken.kind() == Token::Kind::RightParen)
          break;

        if (currentToken.kind() != Token::Kind::Comma)
          throw std::runtime_error("Bad");

        getNextToken();
      }
    }

    getNextToken(); // eat ')'
    return std::make_unique<CallExpressionNode>(name, std::move(arguments));
  }

  std::unique_ptr<Node> parsePrimary() {
    TRACE_METHOD;

    switch (currentToken.kind()) {
    case Token::Kind::Identifier: {
      return parseIdentifierExpression();
    }
    case Token::Kind::Number: {
      std::string value{currentToken.lexeme()};
      getNextToken();
      return std::make_unique<NumericLiteralNode>(std::stoi(value));
    }
    case Token::Kind::LeftParen: {
      return parseParenExpression();
    }
    default:
      throw std::runtime_error(
          "Unexpected token when parsing primary expression.");
    }
  }

  std::unique_ptr<Node> parseExpression() {
    TRACE_METHOD;

    auto lhs = parsePrimary();
    if (!lhs)
      return nullptr;

    return parseBinaryExpression(0, std::move(lhs));
  }

  std::unique_ptr<Node> parseReturnStatement() {
    TRACE_METHOD;

    getNextToken(); // eat 'return'
    auto expression = parseExpression();

    expect(Token::Kind::Semicolon);
    getNextToken();

    return std::make_unique<ReturnStatementNode>(std::move(expression));
  }

  std::unique_ptr<Node> parseIfStatement() {
    TRACE_METHOD;

    getNextToken(); // eat 'if'

    expect(Token::Kind::LeftParen);
    getNextToken();

    auto condition = parseExpression();

    expect(Token::Kind::RightParen);
    getNextToken();

    std::unique_ptr<Node> ifTrue = parseBlock();

    std::unique_ptr<Node> ifFalse = nullptr;

    if (currentToken.kind() == Token::Kind::Else) {
      getNextToken(); // eat 'else'
      ifFalse = parseBlock();
    }

    return std::make_unique<IfStatementNode>(
        std::move(condition), std::move(ifTrue), std::move(ifFalse));
  }

  std::unique_ptr<Node> parseLetStatement() {
    TRACE_METHOD;

    getNextToken(); // eat 'let'

    expect(Token::Kind::Identifier);
    std::string name{currentToken.lexeme()};
    getNextToken();

    expect(Token::Kind::Equals);
    getNextToken();

    auto expression = parseExpression();

    expect(Token::Kind::Semicolon);
    getNextToken();

    return std::make_unique<LetStatementNode>(name, std::move(expression));
  }

  std::unique_ptr<Node> parseExpressionStatement() {
    TRACE_METHOD;

    auto expression = parseExpression();

    expect(Token::Kind::Semicolon);
    getNextToken();

    return std::make_unique<ExpressionStatementNode>(std::move(expression));
  }

  std::unique_ptr<Node> parseStatement() {
    TRACE_METHOD;

    switch (currentToken.kind()) {
    case Token::Kind::If:
      return parseIfStatement();
    case Token::Kind::Return:
      return parseReturnStatement();
    case Token::Kind::Let:
      return parseLetStatement();

    case Token::Kind::LeftParen:
    case Token::Kind::Number:
    case Token::Kind::Identifier:
      return parseExpressionStatement();
    default:
      return nullptr;
    }
  }

  std::unique_ptr<BlockNode> parseBlock() {
    TRACE_METHOD;

    expect(Token::Kind::LeftCurly);

    getNextToken();

    std::vector<std::unique_ptr<Node>> statements;

    while (true) {
      if (auto statement = parseStatement()) {
        statements.push_back(std::move(statement));
      } else {
        break;
      }
    }

    expect(Token::Kind::RightCurly);
    getNextToken();

    return std::make_unique<BlockNode>(std::move(statements));
  }

  std::unique_ptr<ParameterNode> parseParameter() {
    TRACE_METHOD;
    std::string argumentName{currentToken.lexeme()};
    getNextToken();

    expect(Token::Kind::Colon);
    getNextToken(); // eat ':'

    auto type = parseType();

    return std::make_unique<ParameterNode>(argumentName, std::move(type));
  }

  std::vector<std::unique_ptr<ParameterNode>> parseParameters() {
    std::vector<std::unique_ptr<ParameterNode>> parameters;
    if (currentToken.kind() != Token::Kind::RightParen) {
      while (true) {
        auto parameterNode = parseParameter();
        parameters.push_back(std::move(parameterNode));

        if (currentToken.kind() == Token::Kind::RightParen)
          break;

        if (currentToken.kind() != Token::Kind::Comma)
          throw std::runtime_error("Bad");

        getNextToken();
      }
    }
    return parameters;
  }

  std::unique_ptr<FunctionDeclarationNode> parseFunction() {
    TRACE_METHOD;

    getNextToken(); // eat 'function'

    expect(Token::Kind::Identifier);
    std::string name{currentToken.lexeme()};

    getNextToken();
    expect(Token::Kind::LeftParen);

    getNextToken(); // eat '('

    std::vector<std::unique_ptr<ParameterNode>> parameters = parseParameters();

    getNextToken(); // eat ')'

    expect(Token::Kind::Colon);
    getNextToken();

    auto returnType = parseType();

    auto block = parseBlock();

    return std::make_unique<FunctionDeclarationNode>(
        name, std::move(parameters), std::move(returnType), std::move(block));
  }

  std::unique_ptr<SourceFileNode> parseSourceFile() {
    TRACE_METHOD;

    auto p = std::make_unique<SourceFileNode>();
    while (true) {
      switch (currentToken.kind()) {
      case Token::Kind::Function:
        p->functions.push_back(parseFunction());
        break;
      default:
        goto exit_loop;
        break;
      }
    }
  exit_loop:;
    return p;
  }
};