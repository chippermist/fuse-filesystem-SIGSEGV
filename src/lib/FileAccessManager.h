#pragma once

#include <cstdint>
#include <string>

#include "Block.h"
#include "INode.h"
#include "INodeManager.h"
#include "Storage.h"
#include "DirectoryRecord.h"

class FileAccessManager {
  INodeManager *inode_manager;
  Storage *disk;
public:
  FileAccessManager(INodeManager& inode_manager, Storage &storage);
  ~FileAccessManager();

  INode::ID getINodeFromPath(std::string path);

private:
  Block::ID blockAt(const INode& inode, uint64_t offset);
  INode::ID componentLookup(INode::ID cur_inode_num, std::string filename);
  INode::ID directLookup(Block *directory, std::string filename);
  Block::ID indirectBlockAt(Block::ID bid, uint64_t offset, uint64_t size);
};
