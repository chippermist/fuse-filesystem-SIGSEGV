#include <iostream>
#include <vector>

#include "lib/Filesystem.h"

int main(int argc, char** argv) {
  Filesystem fs(argc, argv, true);
  // TODO: Check that everything was saved?

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
