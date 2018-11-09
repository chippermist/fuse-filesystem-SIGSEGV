#ifndef SIGSEGV_INODE_H
#define SIGSEGV_INODE_H

#include <cstdint>
#include "Block.h"

struct INode {
  typedef uint64_t ID;

  INode::ID id;
  uint64_t  size;
  uint64_t  flags;
  uint32_t  uid;
  uint32_t  gid;

  // For a total of 128 bytes:
  Block::ID blocks[12];
};

#endif
