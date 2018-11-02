#include "FreeListBlockManager.h"

FreeListBlockManager::FreeListBlockManager(uint64_t topblock, uint64_t index, MemoryBlockManager& mem_block_manager) {
  this->top_block_num = topblock;
  this->index = index;
  this->mem_block_manager = &mem_block_manager;
}

FreeListBlockManager::~FreeListBlockManager() {}

void FreeListBlockManager::insert(uint64_t block_number) {
  Block* dst = new Block;
  struct DatablockNode *top_block = (struct DatablockNode*) dst;
  if (index + 1 >= ((4096 / sizeof(uint64_t)) - 2)) {
    index = 0;
    if(top_block->prev_block == 0) {
      throw std::out_of_range("There is no prev block.\nAlready at top of free-list.");
    }
    top_block_num = top_block->prev_block;
  } else {
    ++index;
  }
  
  mem_block_manager->get(top_block_num, *dst);
  top_block->free_blocks[index] = block_number;
  mem_block_manager->set(top_block_num, *dst);

  //superblock update
  struct Superblock *super_block = (struct Superblock*) dst;
  mem_block_manager->get(0, *dst);
  super_block->free_list_block = top_block_num;
  super_block->free_list_index = index;
  mem_block_manager->set(top_block_num, *dst);
}

uint64_t FreeListBlockManager::remove() {
  return 0;
}