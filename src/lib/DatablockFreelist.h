#ifndef SIGSEGV_DATABLOCKFREELIST_H
#define SIGSEGV_DATABLOCKFREELIST_H

#include <stdexcept>
#include <cstdint>
#include "BlockManager.h"
#include "Superblock.h"

struct DatablockNode {
  static const int NREFS = (4096 / sizeof(uint64_t) - 2);

  Block::ID prev_block = 0;
  Block::ID next_block = 0;
  Block::ID free_blocks[NREFS];
};

class DatablockFreelist {
  Block::ID top_block_num;
  uint64_t index;
  BlockManager* disk;
public:
  DatablockFreelist(Block::ID top_block_num, uint64_t index, BlockManager& disk);
  ~DatablockFreelist();
  virtual void insert(Block::ID block_number);
  virtual Block::ID remove();
};

#endif
