#include <iodiskeam>
#include "lib/Filesystem.h"
#include "lib/blocks/StackBasedBlockManager.h"
#include "lib/inodes/LinearINodeManager.h"
#include "lib/storage/MemoryStorage.h"

int main(int argc, char** argv) {

  // Read number of blocks on disk
  if (argc < 2) {
    std::cout << "Not Enough Arguments." << std::endl;
  }

  uint64_t nblocks = atoi(argv[1]);
  Storage *disk = new MemoryStorage(nblocks);

  // Get superblock and clear it out
  Block block;
  Superblock* superblock = (Superblock*) &block;
  disk->get(0, block);
  memset(&block, 0, Block::BLOCK_SIZE);

  // Set basic superblock parameters
  superblock->block_size = Block::BLOCK_SIZE;
  superblock->block_count = nblocks;

  // Have approximately 1 inode per 2048 bytes of disk space
  superblock->inode_block_start = 1;
  superblock->inode_block_count = ((nblocks * Block::SIZE) / 2048 * INode::INODE_SIZE) / (Block::BLOCK_SIZE);
  superblock->data_block_start = superblock->inode_block_start + superblock->inode_block_count + 1;
  superblock->data_block_count = superblock->block_count - superblock->data_block_start;

  // Write superblock to disk
  disk->set(0, block);

  // Initialize managers and call mkfs
  LinearINodeManager inode_manager(*disk);
  StackBasedBlockManager block_manager(*disk);
  Filesystem filesystem(block_manager, inode_manager);
  filesystem.mkfs();

  // Debug stuff
  std::cout << "\n\n\n-------------\nTesting mkfs()->reserve()->release()\n-------------\n\n\n";

  std::cout << "\nReserving a block 1\n";
  Block::ID block_id = block_manager.reserve();
  std::cout << "Block Allocated: " << block_id << std::endl;

  std::cout << "\nReleasing a block 1\n";
  block_manager.release(block_id);

  std::cout << "\nReserving a block 1\n";
  block_id = block_manager.reserve();
  std::cout << "Block Allocated: " << block_id << std::endl;

  //block_manager.release(block_id);
  std::cout << "\nReserving a block 2\n";
  Block::ID block_id_2 = block_manager.reserve();
  std::cout << "Block Allocated: " << block_id_2 << std::endl;

  for(int i=0; i<170; ++i) {
    block_id = block_manager.reserve();
    std::cout << "Block Allocated: " << block_id << std::endl;
  }

  std::cout << "\n\n\n-------------\n";
  std::cout << "\033[1;32mSuccess. End of test.\033[0m\n";
  std::cout << "-------------\n\n\n";


  return 0;
}
