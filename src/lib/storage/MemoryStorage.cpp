#include "MemoryStorage.h"

#include <cstring>
#include <stdexcept>

MemoryStorage::MemoryStorage(uint64_t nblocks) {
  data = new char[nblocks * 4096];
  size = nblocks;
}

MemoryStorage::~MemoryStorage() {
  delete [] data;
}

void MemoryStorage::get(Block::ID id, Block& dst) {
  if(id >= size) {
    throw std::length_error("Block read out of range.");
  }

  const char* src = &data[id * 4096];
  std::memcpy(dst.data, src, 4096);
}

void MemoryStorage::set(Block::ID id, const Block& src) {
  if(id >= size) {
    throw std::length_error("Block write out of range.");
  }

  char* dst = &data[id * 4096];
  std::memcpy(dst, src.data, 4096);
}
