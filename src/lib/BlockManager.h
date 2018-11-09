#ifndef SIGSEGV_BLOCKMANAGER_H
#define SIGSEGV_BLOCKMANAGER_H

#include <stdexcept>
#include <cstdint>
#include "Storage.h"
#include "Superblock.h"

struct DatablockNode {
  static const int NREFS = (4096 / sizeof(uint64_t) - 2);

  Block::ID prev_block = 0;
  Block::ID next_block = 0;
  Block::ID free_blocks[NREFS];
};

#define FILE_NAME_MAX_SIZE 28

#define DIR_INODE_INFO_SIZE 32

#define DIR_BLOCK_INODE_COUNT BlockSize/DIR_INODE_INFO_SIZE

class BlockManager {
  Block::ID top_block_num;
  uint64_t index;
  Storage* disk;
public:
  BlockManager(Block::ID top_block_num, uint64_t index, Storage& disk);
  ~BlockManager();
  virtual void insert(Block::ID block_number);
  virtual Block::ID remove();
};

#endif
