#ifndef SIGSEGV_SUPERBLOCK_H
#define SIGSEGV_SUPERBLOCK_H

#include <cstdint>

struct Superblock {
  // Disk Info
  uint64_t block_size;
  uint64_t block_count;

  // INode Info
  uint64_t inode_block_start;
  uint64_t inode_block_count;

  // Data Block Info
  uint64_t data_block_start;
  uint64_t data_block_count;

  uint64_t free_list_block;
  uint64_t free_list_index;
};

#endif
