#include "Filesystem.h"
#include "FSExceptions.h"

#include <cstring>
#include <vector>

struct Writer {
  std::vector<Block::ID> old_blocks;
  std::vector<Block::ID> new_blocks;

  BlockManager* manager;
  const char*   data;
  bool          truncate;
  uint64_t      length;
  uint64_t      offset;

  Writer(): old_blocks(), new_blocks() {
    // All done.
  }

  Block::ID write(Block::ID id, uint64_t scale) {
    if(offset >= scale) {
      offset -= scale;
      return id;
    }

    if(length == 0 && id == 0) {
      return 0;
    }

    Block block;
    if(scale > Block::SIZE) {
      uint64_t nrefs = Block::SIZE / sizeof(Block::ID);
      Block::ID* refs = (Block::ID*) block.data;
      manager->get(id, block);

      scale /= nrefs;
      for(uint64_t i = 0; i < nrefs; ++i) {
        if(offset >= scale) {
          // We're still burning up offset...
          offset -= scale;
        }
        else if(length > 0) {
          // We still have data to write...
          refs[i] = write(refs[i], scale);
        }
        else if(!truncate) {
          // We're past the write - stop if we're not truncating!
          break;
        }
        else if(refs[i] == 0) {
          // This block is already zero, no need to truncate...
          continue;
        }
        else if(scale > Block::SIZE) {
          // We have data in a sub-block - better clear it out!
          refs[i] = write(refs[i], scale);
        }
        else {
          // It's a data block - clean it up:
          old_blocks.push_back(refs[i]);
          refs[i] = 0;
        }
      }
    }
    else {
      if(offset > 0 || length < Block::SIZE) {
        manager->get(id, block);
      }

      uint64_t len = std::min(length, Block::SIZE - offset);
      std::memcpy(block.data + offset, data, len);
      uint64_t end = offset + len;

      if(truncate && end < Block::SIZE) {
        std::memset(block.data + end, 0, Block::SIZE - end);
      }

      data   += len;
      length -= len;
      offset  = 0;
    }

    Block::ID newid = manager->reserve();
    manager->set(newid, block);

    if(id != 0) old_blocks.push_back(id);
    new_blocks.push_back(newid);
    return newid;
  }
};

int Filesystem::write(INode::ID id, const char* data, uint64_t length, uint64_t offset, bool truncate) {
  if(offset > max_file_size || length > max_file_size - offset) {
    throw FileTooBig();
  }

  Writer writer;
  writer.manager  = block_manager;
  writer.data     = data;
  writer.truncate = truncate;
  writer.length   = length;
  writer.offset   = offset;

  INode inode = getINode(id);
  // Write to direct blocks, as needed:
  Block::ID* refs = inode.block_pointers;
  for(uint64_t i = 0; i < INode::DIRECT_POINTERS; ++i) {
    refs[i] = writer.write(refs[i], Block::SIZE);
  }

  // Write to indirect blocks, as needed:
  uint64_t nrefs = Block::SIZE / sizeof(Block::ID);
  uint64_t scale = Block::SIZE * nrefs;
  for(uint64_t i = INode::DIRECT_POINTERS; i < INode::DIRECT_POINTERS + 3; ++i) {
    refs[i] = writer.write(refs[i], scale);
    scale  *= nrefs;
  }

  try {
    inode.size = std::max(inode.size, offset + length);
    if(truncate) inode.size = offset + length;

    inode.atime = time(NULL);
    inode.ctime = inode.atime;
    inode.mtime = inode.atime;
    save(id, inode);
  }
  catch(std::exception& ex) {
    // Commit failed - roll back.
    block_manager->release(writer.new_blocks);
    throw ex;
  }

  // Commit succeeded - release old blocks.
  block_manager->release(writer.old_blocks);
  return length;
}
