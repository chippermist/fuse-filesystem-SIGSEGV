#include "LinearINodeManager.h"

LinearINodeManager::LinearINodeManager(uint64_t num_inodes, Storage& storage)
{
	this->num_inodes = num_inodes;
	this->disk = &storage;
}

LinearINodeManager::~LinearINodeManager() {}

// Get an inode from the freelist and return it
INode::ID LinearINodeManager::reserve(Block &block) {
	uint64_t num_inodes_per_block = Block::BLOCK_SIZE / INode::INODE_SIZE;
	for (uint64_t block_index = 0; block_index < this->num_inodes / num_inodes_per_block; block_index++) {

		// Read in inode block from disk, into the block passed in by user
		this->disk->get(1 + block_index, block);

		// Check each inode in the block and see if it's free
		for (uint64_t inode_index = 0; inode_index < num_inodes_per_block; inode_index++) {
			INode *inode = (INode *) &(block.data[inode_index * INode::INODE_SIZE]);
			if (inode->type == FileType::FREE) {
				return inode_index + num_inodes_per_block * block_index;
			}
		}
	}
	throw std::out_of_range("Can't allocate any more inodes!");
}

// Free an inode and return to the freelist
void LinearINodeManager::release(INode::ID id) {

	// Check if valid id
	if (id >= this->num_inodes) {
		throw std::out_of_range("INode index is out of range!");
	}

	uint64_t block_index = id / Block::BLOCK_SIZE;
	uint64_t inode_index = id % Block::BLOCK_SIZE;

	// Load the inode and modify attribute
	Block block;
	this->disk->get(1 + block_index, block);
	INode *inode = (INode *) &(block.data[inode_index * INode::INODE_SIZE]);
	inode->type = FileType::FREE;

	// Write the inode back to disk
	this->disk->set(1 + block_index, block);
}

// Reads an inode from disk into the memory provided by the user
void LinearINodeManager::get(INode::ID inode_num, INode& user_inode) {

	// Check if valid ID
	if (inode_num >= this->num_inodes) {
		throw std::out_of_range("INode index is out of range!");
	}

	uint64_t block_index = inode_num / Block::BLOCK_SIZE;
	uint64_t inode_index = inode_num % Block::BLOCK_SIZE;

	Block block;
	this->disk->get(1 + block_index, block);
	INode *inode = (INode *) &(block.data[inode_index * INode::INODE_SIZE]);

	memcpy(&user_inode, inode, INode::INODE_SIZE);
}

void LinearINodeManager::set(INode::ID inode_num, const INode& user_inode) {

	// Check if valid ID
	if (inode_num >= this->num_inodes) {
		throw std::out_of_range("INode index is out of range!");
	}

	uint64_t block_index = inode_num / Block::BLOCK_SIZE;
	uint64_t inode_index = inode_num % Block::BLOCK_SIZE;

	Block block;
	this->disk->get(1 + block_index, block);
	INode *inode = (INode *) &(block.data[inode_index * INode::INODE_SIZE]);

	memcpy(inode, &user_inode, INode::INODE_SIZE);
}
