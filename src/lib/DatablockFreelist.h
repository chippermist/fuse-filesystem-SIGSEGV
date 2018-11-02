#ifndef SIGSEGV_DATABLOCKFREELIST_H
#define SIGSEGV_DATABLOCKFREELIST_H

#include <stdexcept>
#include <cstdint>
#include "BlockManager.h"
#include "Superblock.h"

#define NUM_FREE_BLOCKS (4096 / sizeof(uint64_t) - 2)

struct DatablockNode {
  uint64_t prev_block = 0;
  uint64_t next_block = 0;
  uint64_t free_blocks[NUM_FREE_BLOCKS];
};

class DatablockFreelist {
  uint64_t top_block_num;
  uint64_t index;
  BlockManager* disk;
public:
  DatablockFreelist(uint64_t top_block_num, uint64_t index, BlockManager& disk);
  ~DatablockFreelist();
  virtual void insert(uint64_t block_number);
  virtual uint64_t remove();
};

#endif
