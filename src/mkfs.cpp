#include <iostream>
#include "lib/Filesystem.h"
#include "lib/blocks/StackBasedBlockManager.h"
#include "lib/inodes/LinearINodeManager.h"
#include "lib/storage/MemoryStorage.h"

static const uint64_t BLOCK_SIZE = 4096;

int main(int argc, char** argv) {
  // Need to know how to initialize filesystem from mkfs
  // since we don't have a BlockManager or an INodeManager object

  if(argc < 2) {
    std::cout << "Not Enough Arguments." << std::endl;
  }

  Storage *str = new MemoryStorage(atoi(argv[1]));
  Block block;
  Superblock* superblock = (Superblock*) &block;
  str->get(0, block);

  superblock->block_size = BLOCK_SIZE;
  superblock->block_count = atoi(argv[1]);
  superblock->inode_block_start = 1;
  superblock->inode_block_count = 10; //need to be changed
  superblock->data_block_start = superblock->inode_block_start - superblock->inode_block_count + 1;
  superblock->data_block_count = superblock->block_count - superblock->inode_block_count - 1;
  str->set(0, block);

  LinearINodeManager inodes(*str);
  StackBasedBlockManager blocks(*str);

  Filesystem filesystem(blocks, inodes);
  filesystem.mkfs();
  return 0;
}
