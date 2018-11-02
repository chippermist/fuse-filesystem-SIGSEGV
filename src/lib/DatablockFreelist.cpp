#include "DatablockFreelist.h"

DatablockFreelist::DatablockFreelist(uint64_t top_block_num, uint64_t index, BlockManager& block_manager) {
  this->top_block_num = top_block_num;
  this->index = index;
  this->disk = &block_manager;
}

DatablockFreelist::~DatablockFreelist() {}

void DatablockFreelist::insert(uint64_t free_block_num) {
  // Read top block in freelist from disk
  Block* block = new Block;
  struct DatablockNode *node = (struct DatablockNode *) block;
  this->disk->get(this->top_block_num, *block);

  // If insertion causes index to overflow the block,
  // move to previous block in free list.
  if (this->index + 1 >= NUM_FREE_BLOCKS) {
    if (!node->prev_block)
      throw std::out_of_range("Can't insert block at top of data block free list!");
    this->top_block_num = node->prev_block;
    this->disk->get(this->top_block_num, *block);
    index = 0;
  } else {
    index++;
  }

  // Update the top block's free list
  node->free_blocks[this->index] = free_block_num;
  this->disk->set(this->top_block_num, *block);

  // Update superblock
  struct Superblock *super_block = (struct Superblock*) block;
  this->disk->get(0, *block);
  super_block->free_list_block = top_block_num;
  super_block->free_list_index = index;
  this->disk->set(0, *block);
}

uint64_t DatablockFreelist::remove() {

  // Read top block
  Block *block = new Block;
  struct DatablockNode *node = (struct DatablockNode *) block;
  this->disk->get(this->top_block_num, *block);

  // Check if free list is almost empty and refuse allocation of last block
  if (!node->next_block && !this->index) {
      throw std::out_of_range("Can't get any more free blocks - free list is empty!");
  }

  // Get next free block
  uint64_t free_block_num = node->free_blocks[this->index];
  if (!this->index) {
    this->index = NUM_FREE_BLOCKS - 1;
    this->top_block_num = node->next_block;
  } else {
    this->index--;
  }

  // Update superblock
  struct Superblock *super_block = (struct Superblock*) block;
  this->disk->get(0, *block);
  super_block->free_list_block = top_block_num;
  super_block->free_list_index = index;
  this->disk->set(0, *block);
  return free_block_num;
}
