#pragma once

#include "../BlockManager.h"
#include "../Block.h"

struct DatablockNode {
  static const int NREFS = (Block::BLOCK_SIZE / sizeof(Block::ID) - 2);

  Block::ID prev_block = 0;
  Block::ID next_block = 0;
  Block::ID free_blocks[NREFS];
};

class StackBasedBlockManager: public BlockManager {
public:
  StackBasedBlockManager(Block::ID top_block_num, uint64_t index, Storage& disk);
  ~StackBasedBlockManager();
  virtual void release(Block::ID block_number);
  virtual Block::ID reserve();
private:
  Block::ID top_block_num;
  uint64_t index;
  Storage* disk;
};
