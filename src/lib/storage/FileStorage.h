#pragma once

#include "../Storage.h"
#include <fstream>
#include <stdexcept>

class FileStorage: public Storage {
  std::fstream file;
public:
  FileStorage(const char* filename);
  ~FileStorage();

  void get(Block::ID id, Block& dst);
  void set(Block::ID id, const Block& src);
};
