#pragma once

#include "../Storage.h"

class MemoryStorage: public Storage {
  uint64_t size;
  char*    data;
public:
  MemoryStorage(uint64_t nblocks);
  ~MemoryStorage();

  void get(Block::ID id, Block& dst);
  void set(Block::ID id, const Block& src);
};
