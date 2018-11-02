#ifndef SIGSEGV_BLOCKMANAGER_H
#define SIGSEGV_BLOCKMANAGER_H

#include "Block.h"

class BlockManager {
public:
  virtual ~BlockManager() {}

  virtual void get(Block::ID id, Block& dst) = 0;
  virtual void set(Block::ID id, const Block& src) = 0;
};

#endif
