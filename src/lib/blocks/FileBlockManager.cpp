#include "FileBlockManager.h"

FileBlockManager::FileBlockManager(const char* filename): file(filename) {
  // All done.
}

FileBlockManager::~FileBlockManager() {
  file.close();
}

void FileBlockManager::get(Block::ID id, Block& dst) {
  file.seekg(id * 4096);
  file.read(dst.data, 4096);
}

void FileBlockManager::set(Block::ID id, const Block& src) {
  file.seekp(id * 4096);
  file.write(src.data, 4096);
  file.flush();
}
