#ifndef SIGSEGV_BLOCK_H
#define SIGSEGV_BLOCK_H

#include <cstdint>

struct Block {
  static const uint64_t BLOCK_SIZE = 4096;
  typedef uint64_t ID;

  char data[BLOCK_SIZE];
};

#endif
