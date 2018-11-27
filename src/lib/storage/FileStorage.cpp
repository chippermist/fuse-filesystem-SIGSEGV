#include "FileStorage.h"

FileStorage::FileStorage(const char* filename): file(filename) {
  // All done.
}

FileStorage::~FileStorage() {
  file.close();
}

void FileStorage::get(Block::ID id, Block& dst) {
  file.seekg(id * Block::SIZE);
  file.read(dst.data, Block::SIZE);
}

void FileStorage::set(Block::ID id, const Block& src) {
  file.seekp(id * Block::SIZE);
  file.write(src.data, Block::SIZE);
  file.flush();
}
