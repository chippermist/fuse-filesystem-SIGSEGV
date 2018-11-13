#pragma once

#include "Block.h"

class Storage {
public:
  virtual void get(Block::ID id, Block& dst) = 0;
  virtual void set(Block::ID id, const Block& src) = 0;
};
