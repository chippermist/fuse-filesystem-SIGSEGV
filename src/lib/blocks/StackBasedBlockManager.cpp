#include "StackBasedBlockManager.h"
#include <iostream>

// Anonymous namespace for file-local types:
namespace {
  struct DatablockNode {
    static const int NREFS = (Block::BLOCK_SIZE / sizeof(Block::ID) - 2);

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


void StackBasedBlockManager::mkfs() {
  Block block;
  this->disk->get(0,block);
  Superblock* superblock = (Superblock*) &block;

  // Setting up the start and count of blocks from superblock
  // this should be setup before calling mkfs()
  Block::ID start = superblock->data_block_start;
  uint64_t count  = superblock->data_block_count;

  // The intial block will start from 0
  Block::ID prev = 0;
  // If the superblock start was set then curr = start
  Block::ID curr;
  // The first available free block 
  Block::ID free_block = start + count - 1;
  DatablockNode* data = (DatablockNode*) &block;
  memset(&block, 0, Block::BLOCK_SIZE);

  //debugging statements
  std::cout << "-------------\nWithin StackBasedBlockManager::mkfs()\n-------------\n";
  std::cout << "\nCurrent free_block is: " << free_block << std::endl;

  // create the free_blocks[] list
  while(true) {
    data->prev_block = prev;
    data->next_block = free_block - 1;

    // debugging statement
    // std::cout << "Current start is: " << curr << std::endl;

    for(int i = 0; i < DatablockNode::NREFS; ++i) {
      std::cout << "Current free_block is: " << free_block << std::endl;
      // If there is a collision
      // exit condition
      if(free_block == start) {
        data->next_block = 0;
        this->disk->set(curr, block);

        this->disk->get(0, block);
        superblock->data_block_start = curr;
        superblock->data_block_count = i;
        //
        this->disk->set(0, block);
        std::cout << "-------------\nEnd of StackBasedBlockManager::mkfs() by (free_block == curr)\n-------------\n";
        return;
      }

      //otherwise set the first free_block into the free block list
      data->free_blocks[i] = free_block;
      free_block -= 1;
    }

    // write the block to disk
    this->disk->set(curr, block);
    prev = curr;
    curr += 1;
  }
  std::cout << "-------------\nEnd of StackBasedBlockManager::mkfs()\n-------------\n";
}

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
