#include <iostream>
#include <vector>
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
  LinearINodeManager inode_manager(*disk);
  StackBasedBlockManager block_manager(*disk);
  Filesystem filesystem(block_manager, inode_manager, *disk);
  filesystem.mkfs();

  // Debug stuff
  disk->get(0, block);
  std::cout << "INode block count: " << superblock->inode_block_count << std::endl;
  std::cout << "Real data block count: " << superblock->data_block_count << std::endl;
  std::cout << "Size of Inode: " << sizeof(INode) << std::endl;
  // std::cout << "Should be wasting 1-2 data blocks" << std::endl;

  /*
  ******************************************************************
  TESTING BLOCK MANAGER
  ******************************************************************
  */
  // // Reserve, release, reserve
  // std::vector<Block::ID> block_ids;
  // for (size_t i = 0; i < 2048; i++) {
  //   Block::ID id = block_manager.reserve();
  //   std::cout << "Reserved: " << id << std::endl;
  //   block_ids.push_back(id);
  // }

  // for (std::vector<Block::ID>::reverse_iterator i = block_ids.rbegin(); i != block_ids.rend(); ++i ) {
  //   std::cout << "Releasing: " << *i << std::endl;
  //   block_manager.release(*i);
  // }

  // for (size_t i = 0; i < 2048; i++) {
  //   Block::ID id = block_manager.reserve();
  //   std::cout << "Reserved: " << id << std::endl;
  //   assert(id == block_ids[i]);
  // }

  // for (std::vector<Block::ID>::reverse_iterator i = block_ids.rbegin(); i != block_ids.rend(); ++i ) {
  //   block_manager.release(*i);
  // }

  // // Exhaust reserves to see what last block number is
  // for (size_t i = 0; i < nblocks; i++) {
  //   try {
  //     block_manager.reserve();
  //   } catch (const std::exception& e) {
  //     std::cout << "Can't allocate more than " << i+1 << " blocks." << std::endl;
  //     break;
  //    }
  // }

  // std::cout << "\n\n\n-------------\n";
  // std::cout << "\033[1;32mSuccess. End of Block Manager tests.\033[0m\n";
  // std::cout << "-------------\n\n\n";

  /*
  ******************************************************************
  TESTING INODE MANAGER
  ******************************************************************
  */

  // // Reserve, release, reserve
  // std::vector<INode::ID> inode_ids;
  // for (size_t i = 0; i < superblock->inode_block_count * (Block::SIZE / INode::SIZE) - 1; i++) {
  //   INode::ID id = inode_manager.reserve();
  //   std::cout << "Reserved: " << id << std::endl;
  //   inode_ids.push_back(id);

  //   // Mark it as used
  //   INode inode;
  //   inode_manager.get(id, inode);
  //   inode.type = FileType::REGULAR;
  //   inode_manager.set(id, inode);
  // }

  // for (std::vector<INode::ID>::reverse_iterator i = inode_ids.rbegin(); i != inode_ids.rend(); ++i ) {
  //   std::cout << "Releasing: " << *i << std::endl;
  //   inode_manager.release(*i);
  // }

  // for (size_t i = 0; i < superblock->inode_block_count * (Block::SIZE / INode::SIZE) - 1; i++) {
  //   INode::ID id = inode_manager.reserve();
  //   std::cout << "Reserved: " << id << std::endl;
  //   assert(id == inode_ids[i]);

  //   // Mark it as used
  //   INode inode;
  //   inode_manager.get(id, inode);
  //   inode.type = FileType::REGULAR;
  //   inode_manager.set(id, inode);
  // }

  // for (std::vector<INode::ID>::reverse_iterator i = inode_ids.rbegin(); i != inode_ids.rend(); ++i ) {
  //   inode_manager.release(*i);
  // }

  // // Exhaust reserves to see what last inode number is
  // for (size_t i = 0; i < superblock->inode_block_count * (Block::SIZE / INode::SIZE) ; i++) {
  //   try {
  //     INode::ID id = inode_manager.reserve();
  //     // Mark it as used
  //     INode inode;
  //     inode_manager.get(id, inode);
  //     inode.type = FileType::REGULAR;
  //     inode_manager.set(id, inode);
  //   } catch (const std::exception& e) {
  //     std::cout << "Can't allocate more than " << i+1 << " inodes." << std::endl;
  //     break;
  //    }
  // }

  // std::cout << "\n\n\n-------------\n";
  // std::cout << "\033[1;32mSuccess. End of INode manager tests.\033[0m\n";
  // std::cout << "-------------\n\n\n";

  return 0;
}
