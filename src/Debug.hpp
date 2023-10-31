#pragma once

#include <iostream>
#include <ostream>

#if defined(DEBUG)
static std::ostream &debug = std::cout;
#else
class NullBuffer : public std::streambuf {
public:
  int overflow(int c) { return c; }
};

static NullBuffer nullBuffer;
static std::ostream nullStream(&nullBuffer);
static std::ostream &debug = nullStream;
#endif
