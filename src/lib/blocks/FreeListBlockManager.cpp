#include "FreeListBlockManager.h"

FreeListBlockManager::FreeListBlockManager(uint64_t topblock, uint64_t index, MemoryBlockManager& mem_block_manager) {
  this->top_block_num = topblock;
  this->index = index;
  this->mem_block_manager = &mem_block_manager;
}

FreeListBlockManager::~FreeListBlockManager() {}

void FreeListBlockManager::insert(uint64_t block_number) {
  Block* dst = new Block;
  mem_block_manager->get(top_block_num, *dst);
  struct DatablockNode *top_block = (struct DatablockNode*) dst;
  top_block->free_blocks[++index] = block_number;
  mem_block_manager->set(top_block_num, *dst);
}

uint64_t FreeListBlockManager::remove() {
  return 0;
}