#ifndef SIGSEGV_SUPERBLOCK_H
#define SIGSEGV_SUPERBLOCK_H

#include <cstdint>

struct Superblock {
  // TODO:  Probably change this a lot...
  uint64_t block_count;
  uint64_t inode_count;
  uint64_t freelist_block;
  uint64_t freelist_index;
};

#endif
