#pragma once

#include "Block.h"
#include "INode.h"
#include "BlockManager.h"
#include "INodeManager.h"
#include "Storage.h"
#include "Directory.h"

struct statvfs;

class Filesystem {
  BlockManager* block_manager;
  INodeManager* inode_manager;
  uint64_t      max_file_size;
  Storage*      disk;
public:
  Filesystem(BlockManager &block_manager, INodeManager& inode_manager, Storage &storage);
  ~Filesystem();

  void mkfs();
  void statfs(struct statvfs* info);

  int  read(INode::ID file_inode_num, char *buffer, size_t size, size_t offset);
  int  write(INode::ID file_inode_num, const char *buf, size_t size, size_t offset);
  int  truncate(INode::ID file_inode_num, size_t length);
  void unlink(INode::ID id);

  std::string dirname(const char* path_cstring);
  std::string basename(const char* path_cstring);

  Directory getDirectory(INode::ID id);
  Directory getDirectory(const std::string& path);
  INode     getINode(INode::ID id);
  INode     getINode(const std::string& path);
  INode::ID getINodeID(const std::string& path);

  void save(const Directory& directory);
  void save(INode::ID id, const INode& inode);

private:
  Block::ID blockAt(const INode& inode, uint64_t offset);
  INode::ID componentLookup(INode::ID cur_inode_num, std::string filename);
  INode::ID directLookup(Block *directory, std::string filename);
  Block::ID indirectBlockAt(Block::ID bid, uint64_t offset, uint64_t size);
  Block::ID allocateNextBlock(INode& file_inode);
  size_t appendData(INode& file_inode, const char *buf, size_t size, size_t offset, bool null_filler);
  void deallocateLastBlock(INode& file_inode);
};
