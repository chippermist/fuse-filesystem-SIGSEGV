#include "StackBasedBlockManager.h"

// Anonymous namespace for file-local types:
namespace {
  struct DatablockNode {
    static const int NREFS = (Block::SIZE / sizeof(Block::ID) - 2);

    Block::ID prev_block = 0;
    Block::ID next_block = 0;
    Block::ID free_blocks[NREFS];
  };

  struct Config {
    uint64_t  magic; // Magic number to identify this block manager (ignored for now)
    Block::ID block; // Block ID of the top block on the stack
    uint64_t  index; // Index of the first free ref in that block
  };
}

StackBasedBlockManager::StackBasedBlockManager(Storage& storage): disk(&storage) {
  Block block;
  Superblock* superblock = (Superblock*) &block;
  Config* config = (Config*) superblock->data_config;
  this->disk->get(0, block);

  this->top_block_num = config->block;
  this->index         = config->index;
}

StackBasedBlockManager::~StackBasedBlockManager() {}

void StackBasedBlockManager::update_superblock() {
  Block block;
  Superblock* superblock = (Superblock*) &block;
  Config* config = (Config*) superblock->data_config;

  this->disk->get(0, block);
  config->block = this->top_block_num;
  config->index = this->index;
  this->disk->set(0, block);
}

void StackBasedBlockManager::release(Block::ID free_block_num) {
  // Read top block in freelist from disk
  Block block;
  DatablockNode *node = (DatablockNode *) &block;
  this->disk->get(this->top_block_num, block);

  // If insertion causes index to overflow the block,
  // move to previous block in free list.
  if (this->index + 1 >= DatablockNode::NREFS) {
    if (!node->prev_block) {
      throw std::out_of_range("Can't insert block at top of data block free list!");
    }

    this->top_block_num = node->prev_block;
    this->disk->get(this->top_block_num, block);
    index = 0;
  } else {
    index++;
  }

  // Update the free list
  node->free_blocks[this->index] = free_block_num;
  this->disk->set(this->top_block_num, block);
  this->update_superblock();
}

Block::ID StackBasedBlockManager::reserve() {

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

  this->update_superblock();
  return free_block_num;
}
