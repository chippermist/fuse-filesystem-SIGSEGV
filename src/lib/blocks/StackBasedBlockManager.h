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
  virtual void release(Block::ID block_num);
  virtual Block::ID reserve();

  void update_superblock();

private:
  Block::ID top_block_num;
  uint64_t index;
  Storage* disk;
  uint64_t first_block;
  uint64_t last_block;
  uint64_t last_index_free_list;
};
