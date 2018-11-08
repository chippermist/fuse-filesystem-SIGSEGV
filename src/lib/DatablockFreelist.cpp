#include "DatablockFreelist.h"

DatablockFreelist::DatablockFreelist(Block::ID top_block_num, uint64_t index, BlockManager& block_manager) {
  this->top_block_num = top_block_num;
  this->index = index;
  this->disk = &block_manager;
}

DatablockFreelist::~DatablockFreelist() {}

void DatablockFreelist::insert(Block::ID free_block_num) {
  // Read top block in freelist from disk
  Block block;
  DatablockNode *node = (DatablockNode *) &block;
  this->disk->get(this->top_block_num, block);

  // If insertion causes index to overflow the block,
  // move to previous block in free list.
  if (this->index + 1 >= DatablockNode::NREFS) {
    if (!node->prev_block)
      throw std::out_of_range("Can't insert block at top of data block free list!");
    this->top_block_num = node->prev_block;
    this->disk->get(this->top_block_num, block);
    index = 0;
  } else {
    index++;
  }

  // Update the top block's free list
  node->free_blocks[this->index] = free_block_num;
  this->disk->set(this->top_block_num, block);

  // Update superblock
  Superblock *super_block = (Superblock*) &block;
  this->disk->get(0, block);
  super_block->free_list_block = top_block_num;
  super_block->free_list_index = index;
  this->disk->set(0, block);
}

Block::ID DatablockFreelist::remove() {

  // Read top block
  Block block;
  DatablockNode *node = (DatablockNode *) &block;
  this->disk->get(this->top_block_num, block);

  // Check if free list is almost empty and refuse allocation of last block
  if (!node->next_block && !this->index) {
      throw std::out_of_range("Can't get any more free blocks - free list is empty!");
  }

  // Get next free block
  Block::ID free_block_num = node->free_blocks[this->index];
  if (!this->index) {
    this->index = DatablockNode::NREFS - 1;
    this->top_block_num = node->next_block;
  } else {
    this->index--;
  }

  // Update superblock
  Superblock *super_block = (Superblock *) &block;
  this->disk->get(0, block);
  super_block->free_list_block = top_block_num;
  super_block->free_list_index = index;
  this->disk->set(0, block);
  return free_block_num;
}
