#include "LinearINodeManager.h"

#if defined(__linux__)
  #include <sys/statfs.h>
  #include <sys/vfs.h>
#else
  #include <fuse.h>
#endif

LinearINodeManager::LinearINodeManager(Storage& storage): disk(&storage) {
  this->reconfigure();
}

LinearINodeManager::~LinearINodeManager() {
  // Nothing to do.
}

// Initialize inodes during mkfs()
void LinearINodeManager::mkfs() {
  this->reconfigure();

  Block block;
  std::memset(block.data, 0, Block::SIZE);
  for(uint64_t i = 0; i < block_count; ++i) {
    this->disk->set(start_block + i, block);
  }
}

// Reload config information from the superblock
void LinearINodeManager::reconfigure() {
  Block block;
  Superblock* superblock = (Superblock*) &block;
  this->disk->get(0, block);

  assert(Block::SIZE % INode::SIZE == 0);
  uint64_t num_inodes_per_block = Block::SIZE / INode::SIZE;
  this->start_block = superblock->inode_block_start;
  this->block_count = superblock->inode_block_count;
  this->num_inodes  = num_inodes_per_block * this->block_count;
}

// Get an inode from the freelist and return it
INode::ID LinearINodeManager::reserve() {
  Block block;
  uint64_t num_inodes_per_block = Block::SIZE / INode::SIZE;
  for (uint64_t block_index = 0; block_index < block_count; block_index++) {

    // Read in inode block from disk
    this->disk->get(start_block + block_index, block);

    // Check each inode in the block and see if it's free
    for (uint64_t inode_index = 0; inode_index < num_inodes_per_block; inode_index++) {
      INode *inode = (INode *) &(block.data[inode_index * INode::SIZE]);
      if (inode->type == FileType::FREE) {
        return inode_index + num_inodes_per_block * block_index;
      }
    }
  }
  throw std::out_of_range("Can't allocate any more inodes!");
}

// Free an inode and return to the freelist
void LinearINodeManager::release(INode::ID inode_num) {

  // Check if valid id
  if (inode_num >= this->num_inodes || inode_num < this->root) {
    throw std::out_of_range("INode index is out of range!");
  }

  uint64_t num_inodes_per_block = (Block::SIZE / INode::SIZE);
  uint64_t block_index = inode_num / num_inodes_per_block;
  uint64_t inode_index = inode_num % num_inodes_per_block;

  // Load the inode and modify attribute
  Block block;
  this->disk->get(start_block + block_index, block);
  INode *inode = (INode *) &(block.data[inode_index * INode::SIZE]);
  inode->type = FileType::FREE;

  // Write the inode back to disk
  this->disk->set(start_block + block_index, block);
}

// Reads an inode from disk into the memory provided by the user
void LinearINodeManager::get(INode::ID inode_num, INode& user_inode) {

  // Check if valid ID
  if (inode_num >= this->num_inodes || inode_num < this->root) {
    throw std::out_of_range("INode index is out of range!");
  }

  uint64_t num_inodes_per_block = (Block::SIZE / INode::SIZE);
  uint64_t block_index = inode_num / num_inodes_per_block;
  uint64_t inode_index = inode_num % num_inodes_per_block;

  Block block;
  this->disk->get(start_block + block_index, block);
  INode *inode = (INode *) &(block.data[inode_index * INode::SIZE]);

  memcpy(&user_inode, inode, INode::SIZE);
}

void LinearINodeManager::set(INode::ID inode_num, const INode& user_inode) {

  // Check if valid ID
  if (inode_num >= this->num_inodes || inode_num < this->root) {
    throw std::out_of_range("INode index is out of range!");
  }

  uint64_t num_inodes_per_block = (Block::SIZE / INode::SIZE);
  uint64_t block_index = inode_num / num_inodes_per_block;
  uint64_t inode_index = inode_num % num_inodes_per_block;

  Block block;
  this->disk->get(start_block + block_index, block);
  INode *inode = (INode *) &(block.data[inode_index * INode::SIZE]);

  memcpy(inode, &user_inode, INode::SIZE);
  this->disk->set(start_block + block_index, block);
}

INode::ID LinearINodeManager::getRoot() {
  return this->root;
}

void LinearINodeManager::statfs(struct statvfs* info) {
  // Based on http://pubs.opengroup.org/onlinepubs/009604599/basedefs/sys/statvfs.h.html
  // Also see http://man7.org/linux/man-pages/man3/statvfs.3.html
  info->f_files  = num_inodes; // Total number of file serial numbers.
  info->f_ffree  = 42; //TODO! // Total number of free file serial numbers.
  info->f_favail = 42; //TODO! // Number of file serial numbers available to non-privileged process.
}
