#ifndef SIGSEGV_LINEARINODEMANAGER_H
#define SIGSEGV_LINEARINODEMANAGER_H

#include<string.h>
#include <cstdint>
#include "../INodeManager.h"
#include "../blocks/MemoryBlockManager.h"

struct FileBlockInfo
{
	// block number in file system
	Block::ID block_n;

	// byte offset into block
	uint16_t offset_b;

	// bytes of I/O in block
	uint16_t block_io;

	// read ahead block number
	Block::ID read_ahead_block_n;
};

class LinearINodeManager : public INodeManager {
public:
	LinearINodeManager(uint32_t ninodes, MemoryBlockManager& memoryBlockMgr);
	~LinearINodeManager();
	INode::ID reserve();
	void release(INode::ID id);
	void bmap(INode& inode, uint64_t offset, FileBlockInfo& fileBlockInfo);
	void iget(INode::ID inode_n, INode& inode);
	bool namei(char *pathname, INode::ID root_inode_n, INode::ID curr_dir_inode_n, INode& inode);

private:
	uint32_t size;
	char*    data;
	MemoryBlockManager memoryBlockManager;

	uint8_t getIndirectionLevel(uint64_t offset);
	uint8_t getBlockIndex(uint8_t step, uint8_t indirection_level, uint64_t file_offset_block_n);
	Block::ID getDiskBlockNumber(uint8_t index, INode& inode);
	Block::ID getDiskBlockNumber(uint8_t index, Block& block);
	uint64_t getFileBlockNumber(uint8_t step, uint64_t current_offset_block_n);
	void getNextPathNameComponent(char* pathname, uint16_t& index, char *name);
	bool isINodeExists(Block* block, uint16_t start, uint16_t end, char* name, INode::ID inode_n);
};

#endif