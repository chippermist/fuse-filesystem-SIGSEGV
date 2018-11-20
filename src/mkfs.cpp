#include <iostream>
#include "lib/Filesystem.h"
#include "lib/blocks/StackBasedBlockManager.h"
#include "lib/inodes/LinearINodeManager.h"
#include "lib/storage/MemoryStorage.h"

int main(int argc, char** argv) {
  if(argc < 2) {
    std::cout << "Not Enough Arguments." << std::endl;
  }

  uint64_t nblocks = atoi(argv[1]);
  nblocks = 2884106;
  // std::cout << nblocks << std::endl;
  Storage *str = new MemoryStorage(nblocks);

  //Super block needs to be set before the mkfs() functions are called
  Block block;
  Superblock* superblock = (Superblock*) &block;
  str->get(0, block);
  memset(&block, 0, Block::BLOCK_SIZE);

  superblock->block_size = Block::BLOCK_SIZE;
  superblock->block_count = nblocks;

  //unsure of -- very brute force association
  superblock->inode_block_start = 1;
  superblock->inode_block_count = (nblocks * 2 * INode::INODE_SIZE) / (Block::BLOCK_SIZE); //need to be changed based on calculations
  superblock->data_block_start = superblock->inode_block_start + superblock->inode_block_count + 1;
  superblock->data_block_count = superblock->block_count - superblock->data_block_start;


  // debugging statements to check value within superblock
  std::cout << "Current block_size is: " << superblock->block_size << std::endl;
  std::cout << "Current block_count is: " << superblock->block_count << std::endl;
  std::cout << "Current inode_block_start is: " << superblock->inode_block_start << std::endl;
  std::cout << "Current inode_block_count is: " << superblock->inode_block_count << std::endl;
  std::cout << "Current data_block_start is: " << superblock->data_block_start << std::endl;
  std::cout << "Current data_block_count is: " << superblock->data_block_count << std::endl;

  // setting the updated superblock to disk
  str->set(0, (Block&)*superblock);

  LinearINodeManager inodes(*str);
  StackBasedBlockManager blocks(*str);

  Filesystem filesystem(blocks, inodes);
  filesystem.mkfs();

  Block::ID block_id = blocks.reserve();
  std::cout << block_id << std::endl;

  blocks.release(block_id);

  block_id = blocks.reserve();
  std::cout << block_id << std::endl;

  return 0;
}
