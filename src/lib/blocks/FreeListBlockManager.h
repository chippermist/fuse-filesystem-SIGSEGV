#ifndef SIGSEGV_FREELISTBLOCKMANAGER_H
#define SIGSEGV_FREELISTBLOCKMANAGER_H

#include "../BlockManager.h"
#include "MemoryBlockManager.h"
#include "../DatablockFreeList.h"
#include <cstring>

class FreeListBlockManager: public BlockManager {
uint64_t top_block_num;
uint64_t index;
MemoryBlockManager* mem_block_manager;
public:
  FreeListBlockManager(uint64_t topblock, uint64_t index, MemoryBlockManager& mem_block_manager);
  ~FreeListBlockManager();
  virtual void insert(uint64_t block_number);
  virtual uint64_t remove();
};

#endif
