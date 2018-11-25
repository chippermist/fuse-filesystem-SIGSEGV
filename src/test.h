#pragma once

#include "lib/Storage.h"
#include "lib/storage/MemoryStorage.h"
#include "lib/blocks/StackBasedBlockManager.h"
#include "lib/INode.h"
#include "lib/Block.h"
#include <cstdint>
#include <stdexcept>
#include <cstring>
#include <cassert>

class MyBlockManager {
    uint64_t next_block_num = 0;
  public:
		MyBlockManager() {
			next_block_num = 0;
		}
		~MyBlockManager() {

		}
    uint64_t reserve() {
			next_block_num += 1;
			return next_block_num - 1;
		}
};

Block::ID indirectBlockAt(Block::ID bid, uint64_t offset, uint64_t size, Storage *disk) {
  Block block;
  disk->get(bid, block);
  Block::ID* refs = (Block::ID*) &block;

  uint64_t index = offset / size;
  if(size == Block::SIZE) {
    return refs[index];
  }

  uint64_t scale = Block::SIZE / sizeof(Block::ID);
  return indirectBlockAt(refs[index], offset % size, size / scale, disk);
}


Block::ID blockAt(const INode& inode, uint64_t offset, Storage *disk) {
  // This function only gets existing blocks.
  // It might need reworking for writes.
  if (offset >= inode.blocks * Block::SIZE) {
    throw std::out_of_range("Offset greater than file size.");
  }

  if (offset < INode::DIRECT_POINTERS * Block::SIZE) {
    return inode.block_pointers[offset / Block::SIZE];
  }

  uint64_t size  = Block::SIZE;
  uint64_t scale = Block::SIZE / sizeof(Block::ID);
  offset -= INode::DIRECT_POINTERS * Block::SIZE;
  for (int i = 0; i < 3; ++i) {
    if (offset < size * scale) {
      Block::ID bid = inode.block_pointers[INode::DIRECT_POINTERS + i];
      return indirectBlockAt(bid, offset, size, disk);
    }

    size   *= scale;
    offset -= size;
  }

  // If we get here, something has gone very wrong.
  throw std::out_of_range("Offset greater than maximum file size!");
}

Block::ID allocateNextBlock(INode& file_inode, MyBlockManager& block_manager, Storage *disk) {

  size_t scale = Block::SIZE / sizeof(Block::ID);
  size_t logical_blk_num = file_inode.blocks + 1;
  Block::ID data_block_num = 0;

  if (logical_blk_num <= INode::DIRECT_POINTERS) {

    // Direct block

    // 1. Just allocate in inode
    data_block_num = block_manager.reserve();
    file_inode.block_pointers[file_inode.blocks] = data_block_num;

  } else if (logical_blk_num <= INode::DIRECT_POINTERS + scale) {

    // Single-indirect

    // 1. Check if need block for the direct pointers
    if (logical_blk_num == INode::DIRECT_POINTERS + 1) {
      file_inode.block_pointers[INode::DIRECT_POINTERS] = block_manager.reserve();
    }

    // 2. Load in first level block
    Block direct_ptrs_blk;
    Block::ID *direct_ptrs = (Block::ID *) &direct_ptrs_blk;
    disk->get(file_inode.block_pointers[INode::DIRECT_POINTERS], direct_ptrs_blk);

    // 3. Allocate the direct block
    logical_blk_num -= INode::DIRECT_POINTERS;
    data_block_num = block_manager.reserve();
    direct_ptrs[logical_blk_num - 1] = data_block_num;
    disk->set(file_inode.block_pointers[INode::DIRECT_POINTERS], direct_ptrs_blk);

  } else if (logical_blk_num <= INode::DIRECT_POINTERS + scale + (scale * scale)) {

    // Double-indirect

    // 1. Check if need block for single-indirect pointers
    if (logical_blk_num == INode::DIRECT_POINTERS + scale + 1) {
      file_inode.block_pointers[INode::DIRECT_POINTERS + 1] = block_manager.reserve();
    }

    // 2. Load in first level block
    Block single_indirect_ptrs_blk;
    Block::ID *single_indirect_ptrs = (Block::ID *) &single_indirect_ptrs_blk;
    disk->get(file_inode.block_pointers[INode::DIRECT_POINTERS + 1], single_indirect_ptrs_blk);

    // 3. Check if need block for direct pointers
    Block::ID block_idx_in_level = logical_blk_num - INode::DIRECT_POINTERS - scale - 1;
    if (block_idx_in_level % scale == 0) {
      single_indirect_ptrs[block_idx_in_level / scale] = block_manager.reserve();
      disk->set(file_inode.block_pointers[INode::DIRECT_POINTERS + 1], single_indirect_ptrs_blk);
    }

    // 4. Load in second level block
    Block direct_ptrs_blk;
    Block::ID *direct_ptrs = (Block::ID *) &direct_ptrs_blk;
    disk->get(single_indirect_ptrs[block_idx_in_level / scale], direct_ptrs_blk);

    // 5. Allocate the direct block
    data_block_num = block_manager.reserve();
    direct_ptrs[block_idx_in_level % scale] = data_block_num;
    disk->set(single_indirect_ptrs[block_idx_in_level / scale], direct_ptrs_blk);

  } else if (logical_blk_num <= INode::DIRECT_POINTERS + scale + (scale * scale) + (scale * scale * scale)) {
    // Triple-indirect

    // 1. Check if need block for double-indirect pointers
    if (logical_blk_num == INode::DIRECT_POINTERS + scale + (scale * scale) + 1) {
      file_inode.block_pointers[INode::DIRECT_POINTERS + 2] = block_manager.reserve();
    }

    // 2. Load in first level block
    Block double_indirect_ptrs_blk;
    Block::ID *double_indirect_ptrs = (Block::ID *) &double_indirect_ptrs_blk;
    disk->get(file_inode.block_pointers[INode::DIRECT_POINTERS + 2], double_indirect_ptrs_blk);

    // 3. Check if need block for single-indirect pointers
    Block::ID block_idx_in_level = logical_blk_num - INode::DIRECT_POINTERS - scale - scale * scale - 1;
    if (block_idx_in_level % (scale * scale) == 0) {
      double_indirect_ptrs[block_idx_in_level / (scale * scale)] = block_manager.reserve();
      disk->set(file_inode.block_pointers[INode::DIRECT_POINTERS + 2], double_indirect_ptrs_blk);
    }

    // 4. Load in second level block
    Block single_indirect_ptrs_blk;
    Block::ID *single_indirect_ptrs = (Block::ID *) &single_indirect_ptrs_blk;
    disk->get(double_indirect_ptrs[block_idx_in_level / (scale * scale)], single_indirect_ptrs_blk);

    // 5. Check if need block for direct pointers
    size_t block_idx_in_level_two = block_idx_in_level % (scale * scale);
    if (block_idx_in_level_two % scale == 0) {
      single_indirect_ptrs[block_idx_in_level_two / scale] = block_manager.reserve();
      disk->set(double_indirect_ptrs[block_idx_in_level / (scale * scale)], single_indirect_ptrs_blk);
    }

    // 6. Load in third level block
    Block direct_ptrs_blk;
    Block::ID *direct_ptrs = (Block::ID *) &direct_ptrs_blk;
    disk->get(single_indirect_ptrs[block_idx_in_level_two / scale], direct_ptrs_blk);

    // 7. Allocate direct block
    data_block_num = block_manager.reserve();
    direct_ptrs[block_idx_in_level_two % scale] = data_block_num;
    disk->set(single_indirect_ptrs[block_idx_in_level_two / scale], direct_ptrs_blk);

  } else {
    // Can't allocate any more blocks for this file!
    throw std::out_of_range("Reached max number of blocks allocated for a single file!");
  }

  // Update the number of allocated data blocks in this inode
  file_inode.blocks++;
  return data_block_num;
}

/**
 * Appends data to the end of the file, either from buf or just NULL filler.
 * Automatically allocates 1 extra block each time a block is needed.
 *
 * Invariant upon entering the function:
 * - Offset == file size, since we are adding to the end of the file.
 */
size_t appendData(INode& file_inode, char *buf, size_t size, size_t offset, bool null_filler, Storage *disk, MyBlockManager& block_manager) {

  assert(offset == file_inode.size);
  size_t total_written = 0;

  // 1. Fill in the last block already allocated if it has space.
  if (offset % Block::SIZE != 0) {
    Block block;
    Block::ID block_num = blockAt(file_inode, offset, disk);
    disk->get(block_num, block);

    /*
      How many bytes to write?
      a) Write the remainder of the block.
      b) Of course, don't write more than what was asked!
    */
    size_t to_write = Block::SIZE - (offset % Block::SIZE);
    if (to_write > size) {
      to_write = size;
    }

    // Copy the data and write to disk
    if (null_filler) {
      memset(block.data + (offset % Block::SIZE), 0, to_write);
    } else {
      memcpy(block.data  + (offset % Block::SIZE), buf, to_write);
    }
    disk->set(block_num, block);

    // Update offset, buf pointer, and num bytes left to write
    offset += to_write;
    buf += to_write;
    size -= to_write;

    file_inode.size += to_write;
    total_written += to_write;
  }

  if (size == 0) {
    return total_written;
  }

  // 2. Need to allocate new blocks.
  while (size > 0) {

    // Should be block-aligned now
    assert(offset % Block::SIZE == 0);

    // Allocate the next data block

    Block block;
    Block::ID block_num = allocateNextBlock(file_inode, block_manager, disk);


    /*
      How many bytes to write?
      a) Write the whole block (offset should be block-aligned).
      b) Of course, don't write more than what was asked!
    */
    size_t to_write = Block::SIZE;
    if (to_write > size) {
      to_write = size;
    }

    // Copy the data and write to disk
    if (null_filler) {
      memset(block.data, 0, to_write);
    } else {
      memcpy(block.data, buf, to_write);
    }
    disk->set(block_num, block);

    // Update offset, buf pointer, and num bytes left to write
    offset += to_write;
    buf += to_write;
    size -= to_write;

    file_inode.size += to_write;
    total_written += to_write;
  }
  return total_written;
}

size_t read(char *buf, size_t size, size_t offset, Storage *disk, INode& file_inode) {

  if (file_inode.type != FileType::REGULAR) {
    return -1; // File is not a regular file
  }

  if (offset >= file_inode.size) {
    return -1; // Can't begin reading from after file
  }

  // Only read until the end of the file
  if (offset + size > file_inode.size) {
    size = file_inode.size - offset;
  }

  size_t total_read = 0;
  while (size > 0) {

    // Get datablock of current offset
    Block::ID cur_block_id = blockAt(file_inode, offset, disk);
    Block block;
    disk->get(cur_block_id, block);

    /*
      How many bytes to read?
      a) Normally read whole blocks at a time.
      b) If the offset isn't block aligned though
         (which could happen for the first block),
         then only read until the end of the block.
      c) Don't read past the end of the file.
      d) Of course, don't read more than what was asked!
    */
    size_t to_read = Block::SIZE - (offset % Block::SIZE);

    if (offset + to_read > file_inode.size) {
      to_read = file_inode.size - offset;
    }

    if (to_read > size) {
      to_read = size;
    }

    // Copy data from block into buffer
    memcpy(buf, block.data + (offset % Block::SIZE), to_read);

    // Update offset, buf pointer, and num bytes left to read
    offset += to_read;
    buf += to_read;
    size -= to_read;

    total_read += to_read;
  }
  return total_read;
}

/**
 * Writes size bytes from buf into a file, starting at the given offset.
 *
 * Inserts NULL filler if the starting offset is beyond the file's size,
 * and this affects the returned total bytes written.
 *
 * Returns -1 on error.
 * - No such file
 * - Gave name of directory
 * - TODO: Incorrect ownership/permissions
 */
size_t write(std::string path, char *buf, size_t size, size_t offset, Storage *disk, MyBlockManager& block_manager, INode& file_inode) {

  if (file_inode.type != FileType::REGULAR) {
    return -1; // File is not a regular file
  }

  // TODO: Check ownership and permissions

  size_t total_written = 0;
  // 1. If we are overwriting any data in the file, do that first.
  while (offset < file_inode.size && size > 0) {

    // Read in block
    Block block;
    Block::ID block_num = blockAt(file_inode, offset, disk);
    disk->get(block_num, block);

    /*
      How many bytes to write?
      a) Normally write whole blocks at a time.
      b) If the offset isn't block aligned though
         (which could happen for the first block),
         then only write until the end of the block.
      c) Don't write past the end of the file (handled in step 3).
      d) Of course, don't write more than what was asked!
    */
    size_t to_write = Block::SIZE - (offset % Block::SIZE);

    if (offset + to_write > file_inode.size) {
      to_write = file_inode.size - offset;
    }

    if (to_write > size) {
      to_write = size;
    }

    // Copy the data and write to disk
    memcpy(block.data + (offset % Block::SIZE), buf, to_write);
    disk->set(block_num, block);

    // Update offset, buf pointer, and num bytes left to write
    offset += to_write;
    buf += to_write;
    size -= to_write;

    total_written += to_write;
  }

  if (size == 0) {
    return total_written;
  }

  // 2. If the offset > file size, insert NULL filler.
  size_t null_filler = offset - file_inode.size;
  if (null_filler > 0) {
      total_written += appendData(file_inode, buf, null_filler, file_inode.size, true, disk, block_manager);
  }

  // 3. Write actual data from offset (which should be file size).
  total_written += appendData(file_inode, buf, size, offset, false, disk, block_manager);

  // 4. Write back changes to file_inode
  // this->inode_manager->set(file_inode_num, file_inode);
  return total_written;
}
