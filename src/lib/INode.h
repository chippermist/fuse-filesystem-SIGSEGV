#pragma once

#include "Block.h"

#define BLOCK_NUMBER_BYTES (sizeof(Block::ID)/sizeof(uint8_t))
#define DIRECT_BLOCKS_COUNT 10
#define REF_BLOCKS_COUNT 13

// 4096/sizeof(uint64_t)
#define INDIRECT_REF_COUNT 64

enum FileType: uint8_t {
  FREE = 0,
  REGULAR = 1,
  DIRECTORY = 2,
};

struct INode {

  typedef uint32_t ID;
  static const uint64_t INODE_SIZE = 256;

  // Taken from page 6 of http://pages.cs.wisc.edu/~remzi/OSTEP/file-implementation.pdf
  // TODO: Check if the field sizes make sense for us
  uint16_t mode; // can it be read/written/executed?
  uint16_t uid; // who owns this file?
  uint16_t gid; // which group does this file belong to?
  uint32_t time; // what time was the file last accessed?
  uint32_t ctime; // what time was the file created?
  uint32_t mtime; // what time was this file last modified?
  uint16_t links_count; // how many hard links are there to this file?
  uint32_t blocks; // how many blocks have been allocated to this file?
  uint32_t size; // how many bytes are in this file?
  uint32_t flags; // how should our FS use this inode?
  uint8_t type; // what kind of inode is this

  /*
    Max File Size Computation
    10 direct blocks, 1 single indirect block, 1 double indirect block, 1 triple indirect block
    (10 + 1024 + 1024 ** 2 + 1024 ** 3) * 4096 = 4402345713664 ~= pow(2, 42) bytes = 4 TB
   */
  Block::ID block_pointers[REF_BLOCKS_COUNT];
};
