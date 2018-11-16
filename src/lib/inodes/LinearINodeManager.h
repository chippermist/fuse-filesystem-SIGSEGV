#pragma once

#include "../INodeManager.h"
#include "../Storage.h"
#include "../Block.h"

class LinearINodeManager: public INodeManager {
public:
	LinearINodeManager(Storage& storage);
	~LinearINodeManager();

	INode::ID reserve();
	void release(INode::ID id);
	void get(INode::ID id, INode& dst);
	void set(INode::ID id, const INode& src);
	INode::ID getRoot();
	virtual void mkfs() = 0;

private:
	Storage *disk;
	uint64_t num_inodes;
};
