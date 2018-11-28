#include "directory_test.h"

// Compile as
//  g++ -std=c++11 -DFUSE_USE_VERSION=26 -D_FILE_OFFSET_BITS=64 -D_DARWIN_USE_64_BIT_INODE -I/usr/local/include/osxfuse/fuse -losxfuse -L/usr/local/lib directory_test.cpp -o directory_test

LinearINodeManager *inode_manager;
StackBasedBlockManager *block_manager;

void createNestedDirectories(INode::ID parent) {
  srand(time(NULL));
  int count = rand() % 1 + 400;

  for(int k=0; k<count; ++k) {
    Directory new_dir = Directory::mkdir(parent);
    INode::ID inode_id = inode_manager->reserve();
    createNestedDirectories(inode_id);
  }
  return;
}


int main(int argv, char** argc) {
  uint64_t nblocks = (1 + 10 + (1 + 512) + (1 + 512 + 512*512) + (1 + 2 + 512*2 + 512*512*2));
  MemoryStorage *disk = new MemoryStorage(1 + 10 + (1 + 512) + (1 + 512 + 512*512) + (1 + 2 + 512*2 + 512*512*2));    //788496
  // Get superblock and clear it out
  Block block;
  Superblock* superblock = (Superblock*) &block;
  disk->get(0, block);
  memset(&block, 0, Block::SIZE);

  // Set basic superblock parameters
  superblock->block_size = Block::SIZE;
  superblock->block_count = nblocks;

  // Have approximately 1 inode per 2048 bytes of disk space
  superblock->inode_block_start = 1;
  if (((nblocks * Block::SIZE) / 2048 * INode::SIZE) % Block::SIZE == 0) {
    superblock->inode_block_count = ((nblocks * Block::SIZE) / 2048 * INode::SIZE) / Block::SIZE;
  } else {
    superblock->inode_block_count = ((nblocks * Block::SIZE) / 2048 * INode::SIZE) / Block::SIZE + 1;
  }
  superblock->data_block_start = superblock->inode_block_start + superblock->inode_block_count;
  superblock->data_block_count = superblock->block_count - superblock->data_block_start;

  // Write superblock to disk
  disk->set(0, block);

  // Initialize managers and call mkfs
  inode_manager = new LinearINodeManager(*disk);
  block_manager = new StackBasedBlockManager(*disk);
  Filesystem filesystem(*block_manager, *inode_manager);
  filesystem.mkfs();

  //------------------------------------------
  // Begining of test
  //------------------------------------------

  INode::ID inode_id_1 = inode_manager->reserve();

  Directory parent_dir = Directory::mkdir(inode_id_1);
  parent_dir = Directory::get(inode_id_1);

  for(int i=0; i<1000; ++i) {
    createNestedDirectories(parent_dir);
  }





  return 0;
}