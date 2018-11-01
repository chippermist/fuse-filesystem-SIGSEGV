#ifndef SIGSEGV_FILEBLOCKMANAGER_H
#define SIGSEGV_FILEBLOCKMANAGER_H

#include "../BlockManager.h"
#include <fstream>

class FileBlockManager: public BlockManager {
  std::fstream file;
public:
  FileBlockManager(const char* filename);
  ~FileBlockManager();

  void get(Block::ID id, Block& dst);
  void set(Block::ID id, const Block& src);
};

#endif
