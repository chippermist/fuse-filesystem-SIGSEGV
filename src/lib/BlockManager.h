#ifndef SIGSEGV_BLOCKMANAGER_H
#define SIGSEGV_BLOCKMANAGER_H

#include "Block.h"

#define FILE_NAME_MAX_SIZE 28

#define DIR_INODE_INFO_SIZE 32

#define DIR_BLOCK_INODE_COUNT BlockSize/DIR_INODE_INFO_SIZE

class BlockManager {
public:
  virtual ~BlockManager() {}

  virtual void get(Block::ID id, Block& dst) = 0;
  virtual void set(Block::ID id, const Block& src) = 0;
};

#endif
