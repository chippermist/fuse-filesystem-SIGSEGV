#pragma once

#include "Block.h"

class BlockManager {
public:
  BlockManager(Block::ID top_block_num, uint64_t index, Storage& disk);
  ~BlockManager();

  virtual void mkfs() = 0;
  virtual void insert(Block::ID block_number) = 0;
  virtual Block::ID remove() = 0;
};
