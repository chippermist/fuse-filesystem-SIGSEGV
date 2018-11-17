#include <iostream>
#include "lib/Filesystem.h"
#include "lib/blocks/StackBasedBlockManager.h"
#include "lib/inodes/LinearINodeManager.h"
#include "lib/storage/MemoryStorage.h"

int main(int argc, char** argv) {
  // Need to know how to initialize filesystem from mkfs
  // since we don't have a BlockManager or an INodeManager object

  if(argc < 2) {
    std::cout << "Not Enough Arguments." << std::endl;
  }

  Storage *str = new MemoryStorage(atoi(argv[1]));
  LinearINodeManager inodes(*str);
  StackBasedBlockManager blocks(*str);

  Filesystem filesystem(blocks, inodes);
  filesystem.mkfs();
  return 0;
}
