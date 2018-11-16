#include "FileAccessManager.h"

FileAccessManager::FileAccessManager(INodeManager& inode_manager, Storage& storage) {
  this->inode_manager = &inode_manager;
  this->disk = &storage;
}

FileAccessManager::~FileAccessManager() {
  // Nothing to do.
}

Block::ID FileAccessManager::blockAt(const INode& inode, uint64_t offset) {
  // TODO: This function only gets existing blocks.
  // It might need reworking for writes.
  if(offset >= inode.size) {
    throw std::out_of_range("Offset greater than file size.");
  }

  if(offset < INode::DIRECT_POINTERS * Block::SIZE) {
    return inode.block_pointers[offset / Block::SIZE];
  }

  uint64_t size  = Block::SIZE;
  uint64_t scale = Block::SIZE / sizeof(Block::ID);
  for(int i = 0; i < 3; ++i) {
    if(offset < size * scale) {
      Block::ID bid = inode.block_pointers[INode::DIRECT_POINTERS + i];
      return indirectBlockAt(bid, offset, size);
    }

    size   *= scale;
    offset -= size;
  }

  // If we get here, something has gone very wrong.
  throw std::out_of_range("Offset greater than maximum file size!");
}

Block::ID FileAccessManager::indirectBlockAt(Block::ID bid, uint64_t offset, uint64_t size) {
  Block block;
  this->disk->get(bid, block);
  Block::ID* refs = (Block::ID*) &block;

  uint64_t index = offset / size;
  if(size == Block::SIZE) {
    return refs[index];
  }

  uint64_t scale = Block::SIZE / sizeof(Block::ID);
  return indirectBlockAt(refs[index], offset % size, size / scale);
}

/**
 * Returns the INode ID associated with a string path.
 * If the path cannot be found, 0 is returned.
 *
 * @param path: A NULL terminated sequence of characters.
 * @return The INode ID associated with the path, or 0 if not found.
 */
INode::ID FileAccessManager::getINodeFromPath(std::string path) {

  // Handle just root directory
  if (path == "/") {
    return this->inode_manager->getRoot();
  }

  // Split the path into components
  path = path.substr(1);
  size_t pos = std::string::npos;
  INode::ID cur_inode_num = this->inode_manager->getRoot();

  while ((pos = path.find("/")) != std::string::npos) {
    std::string component = path.substr(0, path.find("/"));
    path = path.substr(pos + 1);
    cur_inode_num = componentLookup(cur_inode_num, component);
    if (cur_inode_num == 0) {
      return 0;
    }
  }

  return componentLookup(cur_inode_num, path);
}

/**
 * Searches for a filename using a directory inode.
 * Search looks through all direct and indirect pointers
 * in the block.
 *
 * @param did: The INode ID of the directory.
 * @param filename: A string filename that is being searched for.
 * @return The INode ID of the filename if it is found, 0 otherwise.
 */
INode::ID FileAccessManager::componentLookup(INode::ID did, std::string filename) {
  // Read the directory inode
  INode inode;
  this->inode_manager->get(did, inode);

  uint64_t offset = 0;
  uint64_t max = inode.size + Block::SIZE - 1;
  while(offset < max) {
    Block block;
    this->disk->get(blockAt(inode, offset), block);
    INode::ID iid = directLookup(&block, filename);
    if(iid != 0) return iid;
    offset += Block::SIZE;
  }

  return 0;
}

INode::ID FileAccessManager::directLookup(Block *block, std::string filename) {
  // Check block for the desired filename
  size_t offset = 0;
  while (offset < Block::SIZE) {
    DirectoryRecord *record = (DirectoryRecord *) (((char *) block) + offset);

    // Check if record is unused and shouldn't be checked
    if (record->inode_ID == 0) {
      // Make sure that this isn't the last used entry in the block
      if (offset + record->length == offset) {
        return 0;
      }
      offset += record->length;
      continue;
    }

    // Record is being used - should be checked
    if (record->name == filename) {
      return record->inode_ID;
    }

    // Not the correct filename, check next entry
    offset += record->length;
  }

  // Didn't find filename in this directory
  return 0;
}
