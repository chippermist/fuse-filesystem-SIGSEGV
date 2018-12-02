#pragma once

#include "Block.h"

enum FileType: uint8_t {
  FREE      = 0,
  REGULAR   = 1,
  DIRECTORY = 2,
  SYMLINK   = 3,
  RESERVED  = 255
};

struct INode {

  // TODO: Pad out to actually be 256 bytes! -- DONE
  typedef uint64_t ID;
  static const uint64_t SIZE = 256;
  static const uint64_t REF_BLOCKS_COUNT = 13;
  static const uint64_t DIRECT_POINTERS = 10;
  static const uint64_t SINGLE_INDIRECT_POINTERS = 1;
  static const uint64_t DOUBLE_INDIRECT_POINTERS = 1;
  static const uint64_t TRIPLE_INDIRECT_POINTERS = 1;

  // Taken from page 6 of http://pages.cs.wisc.edu/~remzi/OSTEP/file-implementation.pdf
  // TODO: Check if the field sizes make sense for us
  uint32_t mode;   // can it be read/written/executed?
  uint32_t uid;    // who owns this file?
  uint32_t gid;    // which group does this file belong to?
  uint32_t atime;  // what time was the file last accessed?
  uint32_t ctime;  // what time was the file created?
  uint32_t mtime;  // what time was this file last modified?
  uint16_t links;  // how many hard links are there to this file?
  uint64_t blocks; // how many blocks have been allocated to this file?
  uint64_t size;   // how many bytes are in this file?
  uint32_t flags;  // how should our FS use this inode?
  uint8_t  type;   // what kind of inode is this
  uint64_t dev;    // what kind of device this is

  /*
    Max File Size Computation
    10 direct blocks, 1 single indirect block, 1 double indirect block, 1 triple indirect block
    (10 + 512 + 512 ** 2 + 512 ** 3) * 4096 = 550831693824 ~= pow(2, 39) bytes = 512 GB
   */
  Block::ID block_pointers[REF_BLOCKS_COUNT];
  uint8_t __padding[88]; // padding to 256 bytes

public:
  INode();
  INode(FileType type, uint16_t mode, uint64_t dev = 0);
};

static_assert(sizeof(INode) == 256, "INode must be exactly 256 bytes!");
static_assert(INode::SIZE   == 256, "INode::SIZE is not set correctly!");
