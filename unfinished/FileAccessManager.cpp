#include "FileAccessManager.h"

FileAccessManager::FileAccessManager(INodeManager& inode_manager, Storage& storage)
{
	this->inode_manager = &inode_manager;
	this->disk = &storage;
}

FileAccessManager::~FileAccessManager() {}

/**
 * Given a NULL terminated string path, returns the INode
 * number associated with the file. If the path cannot
 * be found, an error is thrown.
 *
 * @param path: A NULL terminated sequence of characters.
 * @return: The INode number associated with the path.
 */
INode::ID FileAccessManager::getINode(std::string path) {

	// Handle just root directory
	if (path == "/") {
		return this->inode_manager->getRoot();
	}

	// Split the path into components
	path = path.substr(1);
	size_t pos = std::string::npos;
	INode::ID cur_inode_num = this->inode_manager->getRoot();

	while ((pos = path.find("/")) != std::string::npos) {
		std::string component = path.substr(0, path.find("/"));
		path = path.substr(pos + 1);
		cur_inode_num = INodeLookup(cur_inode_num, component);
	}

	cur_inode_num = INodeLookup(cur_inode_num, path);
}

/**
 * Searches a directory INode for a member path string
 * and returns the associated INode ID.
 * @param cur_inode_num: The ID of the directory INode to search.
 * @param filename: The filename or dirname to look for in the directory.
 * @return: The INode ID associated with the path in the directory.
 */
INode::ID FileAccessManager::INodeLookup(INode::ID cur_inode_num, std::string filename) {
	// Read the directory inode
	INode dir_inode;
	this->inode_manager->get(cur_inode_num, dir_inode);

	// Search the directory's direct block pointers
	for (size_t i = 0; i < INode::DIRECT_POINTERS; i++) {

		if (dir_inode.block_pointers[i] == 0) {
			throw std::out_of_range("No such file or directory!");
		}

		// Read in the data block for this directory pointer
		Block block;
		this->disk->get(dir_inode.block_pointers[i], block);

		// Check block for the desired filename
		// TODO: Parse block as list of variable length directory entries.

	}

	// TODO: Check indirect pointers

}

// Read data from a file
//

// Write data to a file
// Write data to an inode
// Write data to a directory
// Given inode ID, write x data to y offset


/*

// TODO: currently supporting directory size of only 1 data block
bool FileAccessManager::namei(char *pathname, INode::ID root_inode_num, INode::ID curr_dir_inode_num, INode& inode) {
	INode working_dir_inode;
	uint16_t index = 0;
	bool is_root = false;
	if (pathname[0] == '/') {
		this->inode_manager->get(root_inode_num, working_dir_inode);
		index++;
		is_root = true;
	}
	else {
		this->inode_manager->get(curr_dir_inode_num, working_dir_inode);
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
			this->disk->get(file_block_info.block_num, block);

			INode::ID inode_n;
			if (isINodeExists(&block, file_block_info.byte_offset, file_block_info.bytes_block_io, next_path_name, inode_n)) {
				memset(&working_dir_inode, 0x00, sizeof(INode));
				this->inode_manager->get(inode_n, working_dir_inode);
			}
			else {
				return false;
			}
		}
	}
	memcpy(&inode, &working_dir_inode, sizeof(INode));
	return true;
}

void FileAccessManager::bmap(INode& inode, uint64_t offset, FileBlockInfo& fileBlockInfo) {
	bool isReadAheadBlockExists = false;
	// calculate logical block number in file
	uint64_t file_offset_block_n = offset/Block::BLOCK_SIZE;

	// calculate start byte in block for I/O
	fileBlockInfo.byte_offset = offset % Block::BLOCK_SIZE;

	// calculate number of bytes to copy to user
	uint64_t total_block_n = (inode.size - 1) / Block::BLOCK_SIZE;
	if (total_block_n > file_offset_block_n) {
		fileBlockInfo.bytes_block_io = Block::BLOCK_SIZE - fileBlockInfo.byte_offset;
		isReadAheadBlockExists = true;
	}
	else {
		fileBlockInfo.bytes_block_io = inode.size - offset;
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

		Block::ID disk_block_indexber;
		if (step == 0) {
			disk_block_indexber = getDiskBlockNumber(index, inode);
		}
		else {
			disk_block_indexber = getDiskBlockNumber(index, block);
		}

		if (indirection_level == 0) {
			fileBlockInfo.block_num = disk_block_indexber;
		}

		this->disk->get(disk_block_indexber, block);

		current_offset_block_n = getFileBlockNumber(step, current_offset_block_n);

		indirection_level--;
		step++;
	}
}

void FileAccessManager::getNextPathNameComponent(char* pathname, uint16_t& index, char *name) {
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

bool FileAccessManager::isINodeExists(Block* block, uint16_t start, uint16_t end, char* name, INode::ID inode_n) {
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

uint64_t FileAccessManager::getFileBlockNumber(uint8_t step, uint64_t current_offset_block_n) {
	switch (step)
	{
	case 0:
		current_offset_block_n -= Block::BLOCK_SIZE * DIRECT_BLOCKS_COUNT;
		break;
	case 1:
		current_offset_block_n -= Block::BLOCK_SIZE * INDIRECT_REF_COUNT;
		break;
	case 2:
		current_offset_block_n -= Block::BLOCK_SIZE * INDIRECT_REF_COUNT * INDIRECT_REF_COUNT;
		break;
	default:
		break;
	}
	return current_offset_block_n;
}

Block::ID FileAccessManager::getDiskBlockNumber(uint8_t index, INode& inode) {
	return inode.block_pointers[index];
}

Block::ID FileAccessManager::getDiskBlockNumber(uint8_t index, Block& block) {
	uint16_t char_index = index * BLOCK_NUMBER_BYTES;
	char block_number_content[BLOCK_NUMBER_BYTES];
	for (uint8_t i = 0; i < BLOCK_NUMBER_BYTES; i++)
	{
		block_number_content[i] = block.data[char_index + i];
	}
	Block::ID disk_block_indexber;
	memcpy(&disk_block_indexber, block_number_content, sizeof(uint64_t));
	return disk_block_indexber;
}

uint8_t FileAccessManager::getBlockIndex(uint8_t step, uint8_t indirection_level, uint64_t offset_block_n) {
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

uint8_t FileAccessManager::getIndirectionLevel(uint64_t offset) {
	uint8_t level = 0;
	uint64_t single_indirect_block_offset = DIRECT_BLOCKS_SIZE;
	uint64_t double_indirect_block_offset = single_indirect_block_offset + SINGLE_INDIRECT_BLOCK_SIZE;
	uint64_t triple_indirect_block_offset = double_indirect_block_offset + DOUBLE_INDIRECT_BLOCK_SIZE;
	uint64_t max_file_offset = triple_indirect_block_offset + TRIPLE_INDIRECT_BLOCK_SIZE;

	if (offset < single_indirect_block_offset) {
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

*/
