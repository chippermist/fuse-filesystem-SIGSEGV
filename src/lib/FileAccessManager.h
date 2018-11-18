#pragma once

#include <cstdint>
#include <string>
#include <stdexcept>
#include <cstring>
#include <cassert>

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

  INode::ID getINodeFromPath(std::string path);
  size_t read(std::string path, char *buffer, size_t size, size_t offset);
  size_t write(std::string path, char *buf, size_t size, size_t offset);

private:
  Block::ID blockAt(const INode& inode, uint64_t offset);
  INode::ID componentLookup(INode::ID cur_inode_num, std::string filename);
  INode::ID directLookup(Block *directory, std::string filename);
  Block::ID indirectBlockAt(Block::ID bid, uint64_t offset, uint64_t size);
  void appendBlock(INode& file_inode);
  size_t appendData(INode& file_inode, char *buf, size_t size, size_t offset, size_t null_filler);

};
