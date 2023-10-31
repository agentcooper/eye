#include <fstream>

#include "File.hpp"

std::string File::getContent(const std::string filePath) {
  std::ifstream inputFile;
  inputFile.open(filePath);
  if (!inputFile.is_open()) {
    throw std::runtime_error("Error opening the file.");
  }
  std::string content((std::istreambuf_iterator<char>(inputFile)),
                      (std::istreambuf_iterator<char>()));
  return content;
}