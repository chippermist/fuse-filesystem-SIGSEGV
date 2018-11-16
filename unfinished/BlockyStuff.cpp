// FileManager Methods
void get(std::string path) {
  INode::ID id = INodeManager::getRoot();

  // TODO: Actually split the path:
  while(name = path.next()) {
    id = get(name, id);
  }

  return id;
}

void get(std::string name, INode::ID iid) {
  INode inode;
  INodeManager::get(iid, inode);

  for(uint64_t offset = 0; offset < inode.size; offset += Block::SIZE) {
    Block block;
    Block::ID bid = inode.blockAt(offset);
    BlockManager::get(bid, block);

    // TODO: This changes depending on the directory layout:
    for(DirEnt* dir = &block; dir->size > 0; dir += dir->size) {
      if(dir->name == name) return dir->inode;
    }
  }

  return 0;
}


// INode Methods
void indirectBlockAt(bid, offset, size) {
  uint64_t othersize = size / Block::SIZE;
  uint64_t index = offset / othersize;

  Block block;
  BlockManager::get(bid, block);
  if(othersize == Block::SIZE) return block[index];
  return indirectBlockAt(block[index], offset % othersize, othersize);
}

void blockAt(const INode& inode, uint64_t offset) {
  // This can probably be a single if + integer division:
  for(int i = 0; i < DIRECT_BLOCKS; ++i) {
    if(offset < Block::SIZE) return i;
    offset -= Block::SIZE;
  }

  uint64_t size = Block::SIZE;
  for(int i = 1; i < 4; ++i) {
    size *= Block::SIZE;
    if(offset < size) {
      Block::ID bid = inode.data[DIRECT_BLOCKS + i - 1];
      return indirectBlockAt(bid, offset, size);
    }

    offset -= size;
  }

  throw "Help!";
}
