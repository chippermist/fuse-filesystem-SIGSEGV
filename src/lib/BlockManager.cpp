#include "BlockManager.h"

BlockManager::BlockManager(Block::ID top_block_num, uint64_t index, Storage& storage) {
  this->top_block_num = top_block_num;
  this->index = index;
  this->disk = &storage;
}

BlockManager::~BlockManager() {}

void BlockManager::insert(Block::ID free_block_num) {
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

// Initialize block on mkfs call
void BlockManager::mkfs() {
  Block block;
  this->disk->get(0, block);
  // get the first block from the disk 
  Superblock* superblock = (Superblock*) &block;
  // create a superblock object

  Block::ID start = superblock->data_block_start;
  uint64_t  count = superblock->data_block_count;
  // retrive current count and start position from superblock

  Block::ID prev = 0;
  Block::ID curr = start;
  Block::ID free_block = start + count - 1;
  DatablockNode* data = (DatablockNode*) &block;

  while(true) {
    data->prev_block = prev;
    data->next_block = curr + 1;

    for(int i = 0; i < DatablockNode::NREFS; ++i) {
      if(free_block == curr) {
        data->next_block = 0;
        disk->set(curr, block);

        disk->get(0, block);
        superblock->free_list_block = curr;
        superblock->free_list_index = i;
        disk->set(0, block);
        return;
      }

      data->free_blocks[i] = free_block;
      free_block -= 1;
    }

    disk->set(curr, block);
    prev = curr;
    curr += 1;
  }
}

Block::ID BlockManager::remove() {

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
