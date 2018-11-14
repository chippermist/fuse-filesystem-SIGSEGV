#pragma once

#include <stdexcept>
#include <cstdint>
#include <string.h>
#include "../INodeManager.h"
#include "../Storage.h"
#include "../Block.h"

class LinearINodeManager: public INodeManager {
public:
	LinearINodeManager(uint64_t num_inodes, Storage& storage);
	~LinearINodeManager();
	INode::ID reserve(Block &block);
	void release(INode::ID id);
	void get(INode::ID inode_num, INode& user_inode);
	void set(INode::ID inode_num, const INode& user_inode);
	INode::ID getRoot();

private:
	Storage *disk;
	uint64_t num_inodes;
};
