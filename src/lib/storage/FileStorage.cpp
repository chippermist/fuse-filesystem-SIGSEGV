#include "FileStorage.h"

FileStorage::FileStorage(const char* filename, uint64_t nblocks): file(filename) {
  this->size = nblocks;
}

FileStorage::~FileStorage() {
  file.close();
}

void FileStorage::get(Block::ID id, Block& dst) {
  if(id >= this->size) {
    throw std::length_error("Block read out of range.");
  }

  file.seekg(id * Block::SIZE);
  file.read(dst.data, Block::SIZE);
}

void FileStorage::set(Block::ID id, const Block& src) {
  if(id >= this->size) {
    throw std::length_error("Block write out of range.");
  }

  file.seekp(id * Block::SIZE);
  file.write(src.data, Block::SIZE);
  file.flush();
}
