#pragma once

#include <cstdint>

struct Superblock {
  uint64_t block_count;
  uint64_t inode_count;
  uint64_t free_list_block;
  uint64_t free_list_index;
};
