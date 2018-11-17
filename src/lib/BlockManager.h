#pragma once

#include "Block.h"
#include "Storage.h"

class BlockManager {
public:
  //BlockManager(Block::ID top_block_num, uint64_t index, Storage& disk);
  virtual ~BlockManager() {}
  virtual void mkfs() = 0;
  virtual void release(Block::ID block_number) = 0;
  virtual Block::ID reserve() = 0;
};
