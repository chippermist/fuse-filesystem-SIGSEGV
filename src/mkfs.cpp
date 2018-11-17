#include "lib/Filesystem.h"
#include "lib/blocks/StackBasedBlockManager.h"
#include "lib/inodes/LinearINodeManager.h"
#include "lib/storage/MemoryStorage.h"

int main() {
  // Need to know how to initialize filesystem from mkfs
  // since we don't have a BlockManager or an INodeManager object
  MemoryStorage storage(2048);
  LinearINodeManager inodes(storage);
  StackBasedBlockManager blocks(storage);

  Filesystem filesystem(blocks, inodes);
  filesystem.mkfs();
  return 0;
}
