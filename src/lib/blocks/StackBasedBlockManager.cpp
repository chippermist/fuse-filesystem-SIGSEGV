#include "StackBasedBlockManager.h"
#include "../FSExceptions.h"

#if defined(__linux__)
  #include <sys/statfs.h>
  #include <sys/vfs.h>
  #include <sys/statvfs.h>
#else
  #include <fuse.h>
#endif

// Anonymous namespace for file-local types:
namespace {
  struct DatablockNode {
    static const int NREFS = (Block::SIZE / sizeof(Block::ID));
    Block::ID free_blocks[NREFS];
  };

  struct Config {
    uint64_t  magic;       // Magic number to identify this block manager (ignored for now)
    Block::ID top_block;   // Block ID of the head block in the free list
    uint64_t  top_index;   // Index of the first free ref in the head block
    uint64_t  last_index;  // Index of the last valid entry in the free list
    Block::ID last_block;  // Block ID of the last block in the free list (leftmost)
    Block::ID first_block; // Block ID of the first block in the free list (rightmost)
  };
}

StackBasedBlockManager::StackBasedBlockManager(Storage& storage): disk(&storage) {
  Block block;
  Superblock* superblock = (Superblock*) &block;
  Config* config = (Config*) superblock->data_config;
  this->disk->get(0, block);

  this->top_block   = config->top_block;
  this->top_index   = config->top_index;
  this->last_index  = config->last_index;
  this->last_block  = config->last_block;
  this->first_block = config->first_block;
}

StackBasedBlockManager::~StackBasedBlockManager() {
  // Nothing to do!
}

void StackBasedBlockManager::get(Block::ID id, Block& dst) {
  disk->get(id, dst);
}

void StackBasedBlockManager::mkfs() {

  // Read superblock
  Block superblock_blk;
  this->disk->get(0, superblock_blk);
  Superblock *superblock = (Superblock *) &superblock_blk;
  Config* config = (Config*) superblock->data_config;
  Block::ID start = superblock->data_block_start;
  uint64_t count  = superblock->data_block_count;

  // The first available free block
  Block block;
  Block::ID free_block = start + count - 1;
  DatablockNode *data = (DatablockNode *) &block;
  Block::ID curr = start;

  // Create the freelist
  while (true) {

    for (int i = DatablockNode::NREFS - 1; i >= 0; --i) {

      // If collision, curr is the current free list block
      // We don't want to allocate free list blocks, so exit.
      if (curr == free_block) {
        if (i != DatablockNode::NREFS - 1) {
          // Write last free list block
          this->disk->set(free_block, block);
          config->last_index = i + 1;
          config->last_block = free_block;
        } else {
          // If we just started looking at this free block,
          // the real last free block was before this one.
          config->last_index = 0;
          config->last_block = free_block + 1;
        }

        // Update superblock
        config->top_block = start + count - 1;
        config->top_index = DatablockNode::NREFS - 1;
        config->first_block = start + count - 1;
        superblock->data_block_count = free_block - start - 1; // we don't allow allocation of last free block
        this->disk->set(0, superblock_blk);

        // Update class members with true values
        this->top_block   = config->top_block;
        this->top_index   = config->top_index;
        this->last_index  = config->last_index;
        this->last_block  = config->last_block;
        this->first_block = config->first_block;
        return;
      }

      // Put curr into the free list
      data->free_blocks[i] = curr;
      curr++;
    }

    // Write the free block to disk
    this->disk->set(free_block, block);
    free_block--;
  }
}

void StackBasedBlockManager::update_superblock() {
  Block block;
  Superblock* superblock = (Superblock*) &block;
  Config* config = (Config*) superblock->data_config;

  this->disk->get(0, block);
  config->top_block = this->top_block;
  config->top_index = this->top_index;
  this->disk->set(0, block);
}

void StackBasedBlockManager::release(Block::ID free_block_num) {

  // If insertion causes index to overflow the block,
  // move to previous block in free list.
  if (this->top_index == DatablockNode::NREFS - 1) {
    if (this->top_block == this->first_block) {
      throw std::out_of_range("Can't insert block at top of data block free list!");
    }

    this->top_block = this->top_block + 1;
    this->top_index = 0;
  } else {
    this->top_index++;
  }

  // Update the free list
  Block block;
  DatablockNode *node = (DatablockNode *) &block;
  this->disk->get(this->top_block, block);
  node->free_blocks[this->top_index] = free_block_num;
  this->disk->set(this->top_block, block);
  this->update_superblock();
}

Block::ID StackBasedBlockManager::reserve() {

  // Check if free list is almost empty and refuse allocation of last block
  if (this->top_block == this->last_block && this->top_index == this->last_index) {
    throw OutOfDataBlocks();
  }

  // Get next free block
  Block block;
  DatablockNode *node = (DatablockNode *) &block;
  this->disk->get(this->top_block, block);
  Block::ID free_block_num = node->free_blocks[this->top_index];
  if (this->top_index == 0) {
    this->top_index = DatablockNode::NREFS - 1;
    this->top_block--;
  } else {
    this->top_index--;
  }

  this->update_superblock();
  return free_block_num;
}

void StackBasedBlockManager::statfs(struct statvfs* info) {
  uint64_t nrefs = DatablockNode::NREFS;
  uint64_t total = (last_block - first_block) * nrefs + last_index;
  uint64_t used  = (top_block  - first_block) * nrefs + top_index;

  // Based on http://pubs.opengroup.org/onlinepubs/009604599/basedefs/sys/statvfs.h.html
  // Also see http://man7.org/linux/man-pages/man3/statvfs.3.html
  info->f_blocks  = total;        // Total number of blocks on file system in units of f_frsize.
  info->f_bfree   = total - used; // Total number of free blocks.
  info->f_bavail  = total - used; // Number of free blocks available to non-privileged process.
}

void StackBasedBlockManager::set(Block::ID id, const Block& src) {
  disk->set(id, src);
}
