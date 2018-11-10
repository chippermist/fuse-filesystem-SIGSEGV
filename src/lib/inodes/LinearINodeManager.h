#ifndef SIGSEGV_LINEARINODEMANAGER_H
#define SIGSEGV_LINEARINODEMANAGER_H

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
	void iget(INode::ID inode_n, INode& user_inode);

private:
	Storage *disk;
	uint64_t num_inodes;
};

#endif
