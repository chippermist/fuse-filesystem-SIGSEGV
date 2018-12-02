#pragma once

#include "../Superblock.h"
#include "../INode.h"
#include "../Storage.h"
#include "../Block.h"

class LinearINodeManager: public INode::Manager {
public:
  LinearINodeManager(Storage& storage);
  ~LinearINodeManager();

  void mkfs();
  void statfs(struct statvfs* info);
  INode::ID getRoot();

  INode get(INode::ID id);
  void  set(const INode& src);

  INode reserve();
  void  release(INode inode);

private:
  static const uint64_t root = 1;

  Storage*  disk;
  Block::ID start_block;
  uint64_t  block_count;
  uint64_t  num_inodes;

  void reload();
};
