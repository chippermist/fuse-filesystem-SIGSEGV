#pragma once

#include <cstdint>
#include <string>
#include <stdexcept>
#include <cstring>
#include <cassert>
#include <stack>
#include <algorithm>

#include "Block.h"
#include "INode.h"
#include "BlockManager.h"
#include "INodeManager.h"
#include "Storage.h"
#include "DirectoryRecord.h"

class FileAccessManager {
  BlockManager *block_manager;
  INodeManager *inode_manager;
  Storage *disk;
public:
  FileAccessManager(BlockManager &block_manager, INodeManager& inode_manager, Storage &storage);
  ~FileAccessManager();

  static INode::ID getINodeFromPath(std::string path);
  int read(std::string path, char *buffer, size_t size, size_t offset);
  int write(std::string path, char *buf, size_t size, size_t offset);
  int truncate(std::string path, size_t length);
  static std::string dirname(std::string path);
  static std::string basename(std::string path);

private:
  static Block::ID blockAt(const INode& inode, uint64_t offset);
  static INode::ID componentLookup(INode::ID cur_inode_num, std::string filename);
  static INode::ID directLookup(Block *directory, std::string filename);
  static Block::ID indirectBlockAt(Block::ID bid, uint64_t offset, uint64_t size);
  Block::ID allocateNextBlock(INode& file_inode);
  size_t appendData(INode& file_inode, char *buf, size_t size, size_t offset, bool null_filler);
  void deallocateLastBlock(INode& file_inode);
};
