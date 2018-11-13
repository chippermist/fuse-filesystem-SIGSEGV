#pragma once

#include "Block.h"

class BlockManager {
public:
  virtual ~BlockManager() {}
  virtual void release(Block::ID block_number) = 0;
  virtual Block::ID reserve() = 0;
};
