#ifndef SIGSEGV_LINEARINODEMANAGER_H
#define SIGSEGV_LINEARINODEMANAGER_H

#include <stdexcept>
#include <cstdint>
#include "../INodeManager.h"
#include "../Storage.h"
#include "../Block.h"

class LinearINodeManager : public INodeManager {
public:
	LinearINodeManager(uint64_t num_inodes, Storage& storage);
	~LinearINodeManager();
	INode::ID reserve(Block &block);
	void release(INode::ID id);

private:
	Storage *disk;
	uint64_t num_inodes;
};

#endif
