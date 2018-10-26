#ifndef SIGSEGV_FILESYSTEM_H
#define SIGSEGV_FILESYSTEM_H

#include "BlockManager.h"
#include "INodeManager.h"

class Filesystem {
  BlockManager& blocks;
  INodeManager& inodes;

public:
  Filesystem(BlockManager& b, INodeManager& i): blocks(b), inodes(i) {
    // All done.
  }

  // FS operations:
  // TODO!
};

#endif
