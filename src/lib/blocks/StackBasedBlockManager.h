#pragma once

#include <stdexcept>
#include <cstdint>
#include <cstring>
#include "../BlockManager.h"
#include "../Storage.h"
#include "../Superblock.h"

class StackBasedBlockManager: public BlockManager {
public:
  StackBasedBlockManager(Storage& disk);
  ~StackBasedBlockManager();

  virtual void mkfs();
  virtual void statfs(struct statvfs* info);

  virtual void get(Block::ID id, Block& dst);
  virtual void set(Block::ID id, const Block& src);

  virtual void release(Block::ID block_num);
  virtual Block::ID reserve();

  void update_superblock();

private:
  Block::ID top_block;
  uint64_t  top_index;
  uint64_t  last_index;
  Block::ID first_block;
  Block::ID last_block;
  Storage*  disk;
};
