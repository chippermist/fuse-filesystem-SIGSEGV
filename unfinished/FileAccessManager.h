#pragma once

#include <cstdint>
#include <string.h>
#include "Block.h"
#include "INode.h"
#include "INodeManager.h"
#include "Storage.h"

#define FILE_NAME_MAX_SIZE 28
#define DIR_INODE_INFO_SIZE 32

struct FileBlockInfo
{
	// block number in file system
	Block::ID block_num;

	// byte offset into block
	uint16_t byte_offset;

	// bytes of I/O in block
	uint16_t bytes_block_io;

	// read ahead block number
	Block::ID read_ahead_block_n;
};

class FileAccessManager {
	INodeManager *inode_manager;
	Storage *disk;
public:
	FileAccessManager(INodeManager& inode_manager, Storage &storage);
	~FileAccessManager();
	INode::ID accessFileByName(char *path);

/*
	void bmap(INode& inode, uint64_t offset, FileBlockInfo& fileBlockInfo);
	bool namei(char *pathname, INode::ID root_inode_n, INode::ID curr_dir_inode_n, INode& inode);

private:
	uint8_t getIndirectionLevel(uint64_t offset);
	uint8_t getBlockIndex(uint8_t step, uint8_t indirection_level, uint64_t file_offset_block_n);
	Block::ID getDiskBlockNumber(uint8_t index, INode& inode);
	Block::ID getDiskBlockNumber(uint8_t index, Block& block);
	uint64_t getFileBlockNumber(uint8_t step, uint64_t current_offset_block_n);
	void getNextPathNameComponent(char* pathname, uint16_t& index, char *name);
	bool isINodeExists(Block* block, uint16_t start, uint16_t end, char* name, INode::ID inode_n);
*/
};
