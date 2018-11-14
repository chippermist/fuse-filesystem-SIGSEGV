#include "FileStorage.h"

FileStorage::FileStorage(const char* filename): file(filename) {
  // All done.
}

FileStorage::~FileStorage() {
  file.close();
}

void FileStorage::get(Block::ID id, Block& dst) {
  file.seekg(id * 4096);
  file.read(dst.data, 4096);
}

void FileStorage::set(Block::ID id, const Block& src) {
  file.seekp(id * 4096);
  file.write(src.data, 4096);
  file.flush();
}
