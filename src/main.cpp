#include <iomanip>
#include <set>
#include <string>

#include "Codegen.hpp"
#include "Debug.hpp"
#include "File.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include "PostProcessor.hpp"
#include "Prelude.hpp"
#include "Printer.hpp"
#include "SymbolTable.hpp"

void printHelp();

int main(int argc, const char *argv[]) {
  if (argc <= 2) {
    printHelp();
    return 1;
  }

  const std::string command{argv[1]};
  const std::string filePath{argv[2]};

  const std::set<std::string> availableCommands{"lex", "parse", "symbols",
                                                "post", "emit"};
  const std::set<std::string> helpCommands{"-h", "--help", "help"};

  if (helpCommands.contains(command)) {
    printHelp();
    return 0;
  }

  if (!availableCommands.contains(command)) {
    std::cout << "Error: unknown command." << std::endl;
    std::cout << std::endl;
    printHelp();
    return 1;
  }

  const auto source = File::getContent(filePath);
  const auto preludeSource = getPrelude();

  Lexer lexer{source};
  Lexer preludeLexer{preludeSource};

  if (command == "lex") {
    lexer.printDebugOutput();
    return 0;
  }

  Parser parser{lexer};
  Parser preludeParser{preludeLexer};
  auto sourceFile = parser.parseSourceFile();
  auto prelude = preludeParser.parseSourceFile();

  if (command == "parse") {
    Printer printer{*sourceFile};
    printer.print();
    return 0;
  }

  // TODO: pick a better name, ScopeCollection?
  SymbolTableVisitor symbolTableVisitor;

  symbolTableVisitor.setInternalMode(true);
  symbolTableVisitor.createSymbolsFromSourceFile(*prelude);
  symbolTableVisitor.setInternalMode(false);

  symbolTableVisitor.createSymbolsFromSourceFile(*sourceFile);

  if (command == "symbols") {
    symbolTableVisitor.print();
    return 0;
  }

  PostProcessor postProcessor{*sourceFile, symbolTableVisitor};
  auto modifiedSourceFile = postProcessor.run();

  if (command == "post") {
    Printer printer{*modifiedSourceFile};
    printer.print();
    return 0;
  }

  for (auto &preludeFunction : prelude->functions) {
    modifiedSourceFile->functions.insert(modifiedSourceFile->functions.begin(),
                                         std::move(preludeFunction));
  }

  Codegen codegen{*modifiedSourceFile, symbolTableVisitor};
  return codegen.compile("program.o");
}

void printHelp() {
  std::cout << "OVERVIEW: Eye compiler." << std::endl;
  std::cout << std::endl;
  std::cout << "USAGE: eye <subcommand> <file>" << std::endl;
  std::cout << std::endl;
  std::cout << "OPTIONS:" << std::endl;
  std::cout << std::setw(26) << std::left << "  -h, --help"
            << "Show help information." << std::endl;
  std::cout << std::endl;
  std::cout << "SUBCOMMANDS:" << std::endl;
  std::cout << std::setw(26) << std::left << "  lex"
            << "Lex and print the token stream." << std::endl;
  std::cout << std::setw(26) << std::left << "  parse"
            << "Parse and print the Abstract Syntax Tree." << std::endl;
  std::cout << std::setw(26) << std::left << "  symbols"
            << "Run semantic analysis and print symbol tables." << std::endl;
  std::cout << std::setw(26) << std::left << "  post"
            << "Run post-processor and print the modified AST." << std::endl;
  std::cout << std::setw(26) << std::left << "  emit"
            << "Emit object file (program.o)." << std::endl;
}