#ifndef SIGSEGV_INODE_H
#define SIGSEGV_INODE_H

#include "Block.h"

#define BLOCK_NUMBER_BYTES sizeof(Block::ID)/sizeof(uint8_t)

#define DIRECT_BLOCKS_COUNT 10

#define REF_BLOCKS_COUNT 13

// 4096/sizeof(uint64_t)
#define INDIRECT_REF_COUNT 64

// 10 * 4096
#define DIRECT_BLOCKS_SIZE 40960

// 64 * 4096
#define SINGLE_INDIRECT_BLOCK_SIZE 262144

// 64 * 64 * 4096
#define DOUBLE_INDIRECT_BLOCK_SIZE 16777216

// 64 * 64 * 64 * 4096
#define TRIPLE_INDIRECT_BLOCK_SIZE 1073741824

enum FileType : int {
	// File type is 0 when it is free
	Free = 0,
	regular = 1,
	directory = 2
};

struct INode {
  static const uint64_t INODE_SIZE = 128;

  typedef uint32_t ID;

  // TODO: not sure what is this ? => inode can be accessed using inode number. Why do we need a inode ID
  //INode::ID id;

  // TODO: Currently supporting only regular and directory type
  FileType type;

  uint64_t  size;

  // For a total of 128 bytes:
  //  10 direct blocks, 1 single indirect block, 1 double indirect block, 1 triple indirect block
  Block::ID blocks[REF_BLOCKS_COUNT];
};

#endif
