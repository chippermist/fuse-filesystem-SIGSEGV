#ifndef SIGSEGV_BLOCK_H
#define SIGSEGV_BLOCK_H

#include <cstdint>

#define BLOCK_SIZE 4096
static uint16_t BlockSize = BLOCK_SIZE;

struct Block {
  typedef uint64_t ID;

  char data[BLOCK_SIZE];
};

#endif
