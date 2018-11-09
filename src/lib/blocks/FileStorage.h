#ifndef SIGSEGV_FILESTORAGE_H
#define SIGSEGV_FILESTORAGE_H

#include "../Storage.h"
#include <fstream>

class FileStorage: public Storage {
  std::fstream file;
public:
  FileStorage(const char* filename);
  ~FileStorage();

  void get(Block::ID id, Block& dst);
  void set(Block::ID id, const Block& src);
};

#endif
