#ifndef SIGSEGV_BLOCK_H
#define SIGSEGV_BLOCK_H

#include <cstdint>

struct Block {
  typedef uint64_t ID;

  char data[4096];
};

#endif
