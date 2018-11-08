#include "LinearINodeManager.h"

LinearINodeManager::LinearINodeManager(uint32_t ninodes, MemoryBlockManager& memoryBlockMgr)
	: memoryBlockManager(memoryBlockMgr) {
	data = new char[ninodes * sizeof(INode)];
	size = ninodes;
}

LinearINodeManager::~LinearINodeManager() {
	delete[] data;
}

INode::ID LinearINodeManager::reserve() {
	return 0;
};

void LinearINodeManager::release(INode::ID id) {

}

void LinearINodeManager::iget(INode::ID inode_n, INode& inode) {
	memcpy(&inode, data + (inode_n * sizeof(INode)), sizeof(INode));
}

// TODO: currently supporting directory size of only 1 data block
bool LinearINodeManager::namei(char *pathname, INode::ID root_inode_n, INode::ID curr_dir_inode_n, INode& inode) {
	INode working_dir_inode;
	uint16_t index = 0;
	bool is_root = false;
	if (pathname[0] == '/') {
		iget(root_inode_n, working_dir_inode);
		index++;
		is_root = true;
	}
	else {
		iget(curr_dir_inode_n, working_dir_inode);
	}

	char next_path_name[FILE_NAME_MAX_SIZE];
	while(true)
	{
		memset(&next_path_name, 0x00, FILE_NAME_MAX_SIZE);
		getNextPathNameComponent(pathname, index, next_path_name);

		if (next_path_name[0] == '\0' || next_path_name[0] == '/') {
			break;
		}

		if (is_root && strcmp(next_path_name, "..") == 0) {
			continue;
		}

		uint64_t offset = 0;

		while (true)
		{
			FileBlockInfo file_block_info;
			bmap(working_dir_inode, offset, file_block_info);

			Block block;
			memoryBlockManager.get(file_block_info.block_n, block);

			INode::ID inode_n;
			if (isINodeExists(&block, file_block_info.offset_b, file_block_info.block_io, next_path_name, inode_n)) {
				memset(&working_dir_inode, 0x00, sizeof(INode));
				iget(inode_n, working_dir_inode);
			}
			else {
				return false;
			}
		}
	}
	memcpy(&inode, &working_dir_inode, sizeof(INode));
}

void LinearINodeManager::bmap(INode& inode, uint64_t offset, FileBlockInfo& fileBlockInfo) {
	bool isReadAheadBlockExists = false;
	// calculate logical block number in file
	uint64_t file_offset_block_n = offset/BlockSize;	

	// calculate start byte in block for I/O
	fileBlockInfo.offset_b = offset % BlockSize;

	// calculate number of bytes to copy to user
	uint64_t total_block_n = (inode.size - 1) / BlockSize;
	if (total_block_n > file_offset_block_n) {
		fileBlockInfo.block_io = BlockSize - fileBlockInfo.offset_b;
		isReadAheadBlockExists = true;
	}
	else {
		fileBlockInfo.block_io = inode.size - offset;
	}

	uint8_t indirection_level = getIndirectionLevel(offset);
	uint8_t step = 0;
	uint64_t current_offset_block_n = file_offset_block_n;
	Block block;
	memset(&block, 0x00, sizeof(block));
	while (true)
	{
		uint8_t index;
		index = getBlockIndex(step, indirection_level, current_offset_block_n);

		Block::ID disk_block_number;
		if (step == 0) {
			disk_block_number = getDiskBlockNumber(index, inode);
		}
		else {
			disk_block_number = getDiskBlockNumber(index, block);
		}

		if (indirection_level == 0) {
			fileBlockInfo.block_n = disk_block_number;
		}

		memoryBlockManager.get(disk_block_number, block);

		current_offset_block_n = getFileBlockNumber(step, current_offset_block_n);

		indirection_level--;
		step++;
	}
}

void LinearINodeManager::getNextPathNameComponent(char* pathname, uint16_t& index, char *name) {
	if (pathname[index] == '\0') {
		return;
	}

	while (pathname[index++] == '/');

	uint8_t i;
	for (i = 0; pathname[index] != '/' || pathname[index] != '\0'; i++, index++)
	{
		name[i] = pathname[index];
	}
	name[i] = '\0';
}

bool LinearINodeManager::isINodeExists(Block* block, uint16_t start, uint16_t end, char* name, INode::ID inode_n) {
	char inode_info[DIR_INODE_INFO_SIZE];
	uint8_t inode_read_n = (end - start) / DIR_INODE_INFO_SIZE;

	for (size_t i = start; i < inode_read_n; i++)
	{
		memset(inode_info, 0x00, DIR_INODE_INFO_SIZE);
		memcpy(inode_info, block + i * DIR_INODE_INFO_SIZE, DIR_INODE_INFO_SIZE);

		if (strcmp(inode_info + sizeof(INode::ID), name) == 0) {
			memcpy(&inode_n, inode_info, sizeof(INode::ID));
			return true;
		}
	}
	return false;
}

uint64_t LinearINodeManager::getFileBlockNumber(uint8_t step, uint64_t current_offset_block_n) {
	switch (step)
	{
	case 0:
		current_offset_block_n -= BlockSize * DIRECT_BLOCKS_COUNT;
		break;
	case 1:
		current_offset_block_n -= BlockSize * INDIRECT_REF_COUNT;
		break;
	case 2:
		current_offset_block_n -= BlockSize * INDIRECT_REF_COUNT * INDIRECT_REF_COUNT;
		break;
	default:
		break;
	}
	return current_offset_block_n;
}

Block::ID LinearINodeManager::getDiskBlockNumber(uint8_t index, INode& inode) {
	return inode.blocks[index];
}

Block::ID LinearINodeManager::getDiskBlockNumber(uint8_t index, Block& block) {
	uint16_t char_index = index * BLOCK_NUMBER_BYTES;
	char block_number_content[BLOCK_NUMBER_BYTES];
	for (uint8_t i = 0; i < BLOCK_NUMBER_BYTES; i++)
	{
		block_number_content[i] = block.data[char_index + i];
	}
	Block::ID disk_block_number;
	memcpy(&disk_block_number, block_number_content, sizeof(uint64_t));
	return disk_block_number;
}

uint8_t LinearINodeManager::getBlockIndex(uint8_t step, uint8_t indirection_level, uint64_t offset_block_n) {
	uint8_t index;
	if (step == 0) {
		if (indirection_level == 0) {
			index = offset_block_n;
		}
		else if (indirection_level == 1) {
			index = 10;
		}
		else if (indirection_level == 2) {
			index = 11;
		}
		else {
			index = 12;
		}
	}
	else {
		if (indirection_level == 0) {
			index = offset_block_n;
		}
		else if (indirection_level == 1) {
			index = offset_block_n / INDIRECT_REF_COUNT;
		}
		else {
			index = offset_block_n / (INDIRECT_REF_COUNT * INDIRECT_REF_COUNT);
		}
	}
	return index;
}


uint8_t LinearINodeManager::getIndirectionLevel(uint64_t offset) {
	uint8_t level = 0;
	uint64_t single_indirect_block_offset = DIRECT_BLOCKS_SIZE;
	uint64_t double_indirect_block_offset = single_indirect_block_offset + SINGLE_INDIRECT_BLOCK_SIZE;
	uint64_t triple_indirect_block_offset = double_indirect_block_offset + DOUBLE_INDIRECT_BLOCK_SIZE;
	uint64_t max_file_offset = triple_indirect_block_offset + TRIPLE_INDIRECT_BLOCK_SIZE;

	if (offset >= 0 && offset < single_indirect_block_offset) {
		level = 0;
	}
	else if (offset >= single_indirect_block_offset && offset < double_indirect_block_offset) {
		level = 1;
	}
	else if (offset >= double_indirect_block_offset && offset < triple_indirect_block_offset) {
		level = 2;
	}
	else if (offset >= triple_indirect_block_offset && offset <= max_file_offset) {
		level = 3;
	}
	return level;
}