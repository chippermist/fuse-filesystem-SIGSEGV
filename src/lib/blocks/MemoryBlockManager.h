#ifndef SIGSEGV_MEMORYBLOCKMANAGER_H
#define SIGSEGV_MEMORYBLOCKMANAGER_H

#include "../BlockManager.h"

class MemoryBlockManager: public BlockManager {
  uint64_t size;
  char*    data;
public:
  MemoryBlockManager(uint64_t nblocks);
  ~MemoryBlockManager();

  void get(Block::ID id, Block& dst);
  void set(Block::ID id, const Block& src);
};

#endif
