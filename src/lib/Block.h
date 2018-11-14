#pragma once

#include <cstdint>

struct Block {
  static const uint16_t BLOCK_SIZE = 4096;
  typedef uint64_t ID;

  char data[BLOCK_SIZE];
};
