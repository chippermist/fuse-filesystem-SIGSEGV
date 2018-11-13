#pragma once

#include <stdexcept>
#include <cstdint>
#include "Storage.h"
#include "Superblock.h"

class BlockManager {
public:
  virtual void release(Block::ID block_number) = 0;
  virtual Block::ID reserve() = 0;
};
