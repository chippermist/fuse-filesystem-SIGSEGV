#include "lib/Filesystem.h"
#include "lib/blocks/StackBasedBlockManager.h"
#include "lib/inodes/LinearINodeManager.h"
#include "lib/storage/MemoryStorage.h"

int main(int argc, char** argv) {
  // Need to know how to initialize filesystem from mkfs
  // since we don't have a BlockManager or an INodeManager object
  Storage *str = new MemoryStorage(2048);
  LinearINodeManager inodes(*str);
  StackBasedBlockManager blocks(*str);

  Filesystem filesystem(blocks, inodes);
  filesystem.mkfs();
  return 0;
}
