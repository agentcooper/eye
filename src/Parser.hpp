#pragma once

#include <iostream>
#include <map>
#include <regex>
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

  void setPrecedence(const Token::Kind kind, const int rank) {
    precedence[kind] = 100 - rank;
  }

public:
  Parser(Lexer &lexer)
      : lexer(lexer), currentToken(lexer.getNextToken()),
        nextToken(lexer.getNextToken()) {

    // https://en.wikipedia.org/wiki/Order_of_operations#Programming_languages
    setPrecedence(Token::Kind::Dot, 1);

    setPrecedence(Token::Kind::Asterisk, 3);
    setPrecedence(Token::Kind::Slash, 3);
    setPrecedence(Token::Kind::Percent, 3);

    setPrecedence(Token::Kind::Plus, 4);
    setPrecedence(Token::Kind::Minus, 4);

    setPrecedence(Token::Kind::LessThan, 6);
    setPrecedence(Token::Kind::GreaterThan, 6);
    setPrecedence(Token::Kind::LessThanEquals, 6);
    setPrecedence(Token::Kind::GreaterThanEquals, 6);

    setPrecedence(Token::Kind::DoubleEquals, 7);
    setPrecedence(Token::Kind::ExclamationMarkEquals, 7);

    setPrecedence(Token::Kind::AmpersandAmpersand, 11);

    setPrecedence(Token::Kind::BarBar, 12);

    setPrecedence(Token::Kind::Equals, 14);
    setPrecedence(Token::Kind::PlusEquals, 14);
    setPrecedence(Token::Kind::MinusEquals, 14);
    setPrecedence(Token::Kind::AsteriskEquals, 14);
    setPrecedence(Token::Kind::SlashEquals, 14);
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

      if (currentToken.kind() == Token::Kind::LessThan) {
        getNextToken(); // eat '<'
        auto typeParameters = parseTypeParameters();
        getNextToken(); // eat '>'

        return std::make_unique<TypeReferenceNode>(std::move(identifierNode),
                                                   std::move(typeParameters));
      }

      return std::make_unique<TypeReferenceNode>(std::move(identifierNode));
    }

    if (currentToken.kind() == Token::Kind::LeftParen) {
      getNextToken(); // eat '('

      auto parameters = parseParameters();

      getNextToken(); // eat ')'

      getNextToken(); // eat '=>'

      auto returnType = parseType();

      return std::make_unique<FunctionTypeNode>(std::move(parameters),
                                                std::move(returnType));
    }

    if (currentToken.kind() == Token::Kind::LeftCurly) {
      getNextToken(); // eat '{'
      auto members = parseMembers();
      getNextToken(); // eat '}'
      return std::make_unique<StructTypeNode>(std::move(members));
    }

    auto literal = parsePrimary();
    return std::make_unique<LiteralTypeNode>(std::move(literal));
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

  std::unique_ptr<Node> parsePropertyAssignment() {
    TRACE_METHOD;
    std::string name{currentToken.lexeme()};
    getNextToken();

    expect(Token::Kind::Colon);
    getNextToken(); // eat ':'

    auto initializer = parsePrimary();

    return std::make_unique<PropertyAssignmentNode>(name,
                                                    std::move(initializer));
  }

  std::vector<std::unique_ptr<Node>> parseProperties() {
    TRACE_METHOD;

    std::vector<std::unique_ptr<Node>> properties;
    if (currentToken.kind() != Token::Kind::RightCurly) {
      while (true) {
        auto property = parsePropertyAssignment();
        properties.push_back(std::move(property));

        if (currentToken.kind() == Token::Kind::RightCurly)
          break;

        if (currentToken.kind() != Token::Kind::Comma)
          throw std::runtime_error("Bad");

        getNextToken();
      }
    }
    return properties;
  }

  std::unique_ptr<Node> parseObjectLiteral() {
    TRACE_METHOD;

    getNextToken(); // eat '{'

    auto properties = parseProperties();

    getNextToken(); // eat '}'

    return std::make_unique<ObjectLiteralNode>(std::move(properties));
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

      auto RHS = parsePrimaryPlus();
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

    if (currentToken.kind() == Token::Kind::LeftParen) {
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

    return std::make_unique<IdentifierNode>(name);
  }

  std::unique_ptr<Node> parsePrimary() {
    TRACE_METHOD;

    switch (currentToken.kind()) {
    case Token::Kind::Minus:
    case Token::Kind::Asterisk:
    case Token::Kind::ExclamationMark:
      return parseUnaryExpression();
    case Token::Kind::True:
    case Token::Kind::False: {
      std::string value{currentToken.lexeme()};
      getNextToken();
      return std::make_unique<BooleanLiteralNode>(value == "true");
    }
    case Token::Kind::Identifier: {
      return parseIdentifierExpression();
    }
    case Token::Kind::Integer: {
      std::string value{currentToken.lexeme()};
      getNextToken();
      return std::make_unique<NumericLiteralNode>(std::stod(value), false);
    }
    case Token::Kind::FloatingPoint: {
      std::string value{currentToken.lexeme()};
      getNextToken();
      return std::make_unique<NumericLiteralNode>(std::stod(value), true);
    }
    case Token::Kind::Char: {
      std::string value{currentToken.lexeme()};
      getNextToken();
      if (value[1] == '\\' && value[2] == 'n') {
        return std::make_unique<CharLiteralNode>('\n');
      }
      return std::make_unique<CharLiteralNode>(value[1]);
    }
    case Token::Kind::String: {
      std::string value{currentToken.lexeme()};
      getNextToken();

      auto rawValue = value.substr(1, value.size() - 2);
      auto escapedValue =
          std::regex_replace(rawValue, std::regex(R"(\\n)"), "\n");
      return std::make_unique<StringLiteralNode>(escapedValue);
    }
    case Token::Kind::LeftCurly: {
      return parseObjectLiteral();
    }
    case Token::Kind::LeftParen: {
      return parseParenExpression();
    }
    default:
      throw std::runtime_error(
          "Unexpected token when parsing primary expression.");
    }
  }

  std::unique_ptr<Node> parseUnaryExpression() {
    TRACE_METHOD;

    Token::Kind op = currentToken.kind();
    getNextToken();

    auto expression = parsePrimaryPlus();
    return std::make_unique<UnaryExpressionNode>(op, std::move(expression));
  }

  std::unique_ptr<Node> parsePrimaryPlus() {
    TRACE_METHOD;

    auto lhs = parsePrimary();
    if (!lhs) {
      return nullptr;
    }

    while (true) {
      if (currentToken.kind() == Token::Kind::LeftSquareBracket) {
        getNextToken(); // eat '['

        auto argumentExpression = parseExpression();

        getNextToken(); // eat ']'

        lhs = std::make_unique<ElementAccessExpressionNode>(
            std::move(lhs), std::move(argumentExpression));
      } else {
        break;
      }
    }

    return lhs;
  }

  std::unique_ptr<Node> parseExpression() {
    TRACE_METHOD;

    auto lhs = parsePrimaryPlus();
    if (!lhs) {
      return nullptr;
    }

    return parseBinaryExpression(0, std::move(lhs));
  }

  std::unique_ptr<Node> parseReturnStatement() {
    TRACE_METHOD;

    getNextToken(); // eat 'return'
    if (currentToken.kind() == Token::Kind::Semicolon) {
      getNextToken();
      return std::make_unique<ReturnStatementNode>(nullptr);
    }

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

  std::unique_ptr<Node> parseForStatement() {
    TRACE_METHOD;

    expect(Token::Kind::For);
    getNextToken();

    expect(Token::Kind::LeftParen);
    getNextToken();

    auto initializer = parseLetStatement();

    // expect(Token::Kind::Semicolon);
    // getNextToken();

    auto condition = parseExpression();

    expect(Token::Kind::Semicolon);
    getNextToken();

    auto incrementer = parseExpression();

    expect(Token::Kind::RightParen);
    getNextToken();

    auto body = parseBlock();

    return std::make_unique<ForStatementNode>(
        std::move(initializer), std::move(condition), std::move(incrementer),
        std::move(body));
  }

  std::unique_ptr<Node> parseLetStatement() {
    TRACE_METHOD;

    std::unique_ptr<Node> type = nullptr;
    std::unique_ptr<Node> expression = nullptr;

    getNextToken(); // eat 'let'

    expect(Token::Kind::Identifier);
    std::string name{currentToken.lexeme()};
    getNextToken();

    if (currentToken.kind() == Token::Kind::Colon) {
      getNextToken();
      type = parseType();
    }

    if (currentToken.kind() == Token::Kind::Equals) {
      getNextToken();
      expression = parseExpression();
    }

    expect(Token::Kind::Semicolon);
    getNextToken();

    return std::make_unique<LetStatementNode>(name, std::move(type),
                                              std::move(expression));
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
    case Token::Kind::For:
      return parseForStatement();

    case Token::Kind::LeftParen:
    case Token::Kind::Asterisk:
    case Token::Kind::Integer:
    case Token::Kind::FloatingPoint:
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

  std::unique_ptr<PropertySignatureNode> parsePropertySignature() {
    TRACE_METHOD;

    std::string argumentName{currentToken.lexeme()};
    getNextToken();

    expect(Token::Kind::Colon);
    getNextToken(); // eat ':'

    auto type = parseType();

    return std::make_unique<PropertySignatureNode>(argumentName,
                                                   std::move(type));
  }

  std::vector<std::unique_ptr<Node>> parseTypeParameters() {
    TRACE_METHOD;

    std::vector<std::unique_ptr<Node>> parameters;
    if (currentToken.kind() != Token::Kind::LessThan) {
      while (true) {
        auto type = parseType();
        parameters.push_back(std::move(type));

        if (currentToken.kind() == Token::Kind::GreaterThan)
          break;

        if (currentToken.kind() != Token::Kind::Comma)
          throw std::runtime_error("Bad");

        getNextToken();
      }
    }
    return parameters;
  }

  std::vector<std::unique_ptr<Node>> parseParameters() {
    TRACE_METHOD;

    std::vector<std::unique_ptr<Node>> parameters;
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

  std::vector<std::unique_ptr<Node>> parseMembers() {
    TRACE_METHOD;

    std::vector<std::unique_ptr<Node>> members;
    if (currentToken.kind() != Token::Kind::RightCurly) {
      while (true) {
        auto member = parsePropertySignature();
        members.push_back(std::move(member));

        if (currentToken.kind() == Token::Kind::RightCurly)
          break;

        if (currentToken.kind() != Token::Kind::Comma)
          throw std::runtime_error("Bad");

        getNextToken();
      }
    }
    return members;
  }

  std::unique_ptr<InterfaceDeclarationNode> parseInterfaceDeclaration() {
    TRACE_METHOD;

    getNextToken(); // eat 'interface'

    expect(Token::Kind::Identifier);
    std::string name{currentToken.lexeme()};

    getNextToken();
    expect(Token::Kind::LeftCurly);

    getNextToken(); // eat '{'

    auto members = parseMembers();

    getNextToken(); // eat '}'

    return std::make_unique<InterfaceDeclarationNode>(name, std::move(members));
  }

  std::unique_ptr<FunctionDeclarationNode> parseFunction(bool isDeclaration) {
    TRACE_METHOD;

    getNextToken(); // eat 'function'

    expect(Token::Kind::Identifier);
    std::string name{currentToken.lexeme()};

    getNextToken();

    std::vector<std::string> typeParameters;
    if (currentToken.kind() == Token::Kind::LessThan) {
      getNextToken();
      std::string name{currentToken.lexeme()};
      typeParameters.push_back(name);
      getNextToken();

      expect(Token::Kind::GreaterThan);
      getNextToken();
    }

    expect(Token::Kind::LeftParen);

    getNextToken(); // eat '('

    auto parameters = parseParameters();

    getNextToken(); // eat ')'

    expect(Token::Kind::Colon);
    getNextToken();

    auto returnType = parseType();

    if (isDeclaration) {
      return std::make_unique<FunctionDeclarationNode>(
          name, typeParameters, std::move(parameters), std::move(returnType),
          nullptr);
    }

    auto block = parseBlock();

    return std::make_unique<FunctionDeclarationNode>(
        name, typeParameters, std::move(parameters), std::move(returnType),
        std::move(block));
  }

  std::unique_ptr<SourceFileNode> parseSourceFile() {
    TRACE_METHOD;

    std::vector<std::unique_ptr<Node>> functions;
    std::vector<std::unique_ptr<Node>> interfaces;

    while (true) {
      switch (currentToken.kind()) {

      case Token::Kind::Function:
        functions.push_back(parseFunction(false));
        break;
      case Token::Kind::Declare:
        getNextToken(); // eat 'declare'
        functions.push_back(parseFunction(true));
        expect(Token::Kind::Semicolon);
        getNextToken(); // eat ';'
        break;
      case Token::Kind::Interface:
        interfaces.push_back(parseInterfaceDeclaration());
        break;
      default:
        goto exit_loop;
        break;
      }
    }
  exit_loop:;
    return std::make_unique<SourceFileNode>(std::move(functions),
                                            std::move(interfaces));
  }
};