#pragma once

#include "Block.h"

struct Superblock {
  union {
    uint64_t config[16];
    struct {
      uint32_t  magic;
      uint64_t  block_size;
      uint64_t  block_count;

      Block::ID inode_block_start;
      uint64_t  inode_block_count;

      Block::ID data_block_start;
      uint64_t  data_block_count;
    };
  };

  uint64_t inode_config[8];
  uint64_t data_config[8];
};
