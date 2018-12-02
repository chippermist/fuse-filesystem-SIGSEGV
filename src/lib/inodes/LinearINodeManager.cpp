#include "LinearINodeManager.h"
#include "../FSExceptions.h"

#include <cassert>
#include <cstring>
#include <stdexcept>

#if defined(__linux__)
  #include <sys/statfs.h>
  #include <sys/statvfs.h>
  #include <sys/vfs.h>
#else
  #include <fuse.h>
#endif


LinearINodeManager::LinearINodeManager(Storage& storage): disk(&storage) {
  this->reload();
}

LinearINodeManager::~LinearINodeManager() {
  // Nothing to do.
}

void LinearINodeManager::mkfs() {
  this->reload();

  Block block;
  std::memset(block.data, 0, Block::SIZE);
  for(uint64_t i = 1; i < block_count; ++i) {
    this->disk->set(start_block + i, block);
  }

  // Reserve INodes for null and root:
  INode::Data* inodes = (INode::Data*) block.data;
  inodes[0].type = FileType::RESERVED;
  inodes[1].type = FileType::RESERVED;
  this->disk->set(start_block, block);
}

// Get an inode from the freelist and return it
INode LinearINodeManager::reserve() {
  Block block;
  uint64_t num_inodes_per_block = Block::SIZE / INode::SIZE;
  for(uint64_t i = 0; i < block_count; i++) {
    this->disk->get(start_block + i, block);
    INode::Data* inodes = (INode::Data*) block.data;

    for (uint64_t j = 0; j < num_inodes_per_block; j++) {
      if(inodes[j].type == FileType::FREE) {
        return INode(&inodes[j]);
      }
    }
  }

  throw OutOfINodes();
}

// Free an inode and return to the freelist
void LinearINodeManager::release(INode inode) {
  inode.save([](INode::Data& data) {
    std::memset(&data, 0, INode::SIZE);
  });
}

void LinearINodeManager::reload() {
  Block block;
  Superblock* superblock = (Superblock*) &block;
  this->disk->get(0, block);

  assert(Block::SIZE % INode::SIZE == 0);
  uint64_t num_inodes_per_block = Block::SIZE / INode::SIZE;
  start_block = superblock->inode_block_start;
  block_count = superblock->inode_block_count;
  num_inodes  = num_inodes_per_block * block_count;
}

// Reads an inode from disk into the memory provided by the user
INode LinearINodeManager::get(INode::ID id) {
  if (id >= this->num_inodes || id < this->root) {
    throw std::out_of_range("INode index is out of range!");
  }

  uint64_t num_inodes_per_block = (Block::SIZE / INode::SIZE);
  uint64_t block_index = id / num_inodes_per_block;
  uint64_t inode_index = id % num_inodes_per_block;

  Block block;
  this->disk->get(start_block + block_index, block);

  INode::Data* inodes = (INode::Data*) block.data;
  return INode(&inodes[inode_index]);
}

void LinearINodeManager::set(const INode& inode) {
  if(inode.id() >= this->num_inodes || inode.id() < this->root) {
    throw std::out_of_range("INode index is out of range!");
  }

  uint64_t num_inodes_per_block = (Block::SIZE / INode::SIZE);
  uint64_t block_index = inode.id() / num_inodes_per_block;
  uint64_t inode_index = inode.id() % num_inodes_per_block;

  Block block;
  this->disk->get(start_block + block_index, block);

  inode.read([=](const INode::Data& data) {
    INode::Data* inodes = (INode::Data*) block.data;
    std::memcpy(&inodes[inode_index], &data, INode::SIZE);
  });

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
