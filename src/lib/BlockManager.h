#pragma once

#include "Block.h"
#include "Storage.h"

#if defined(__linux__)
  #include <sys/statvfs.h>
#else
  #include <fuse.h>
#endif

struct statvfs;

class BlockManager {
public:
  //BlockManager(Block::ID top_block_num, uint64_t index, Storage& disk);
  virtual ~BlockManager() {}

  virtual void mkfs() = 0;
  virtual void statfs(struct statvfs* info) = 0;

  virtual void get(Block::ID id, Block& dst) = 0;
  virtual void set(Block::ID id, const Block& src) = 0;

  virtual void release(Block::ID block_number) = 0;
  virtual Block::ID reserve() = 0;
};
