#include "FileAccessManager.h"

FileAccessManager::FileAccessManager(INodeManager& inode_manager, Storage& storage)
{
	this->inode_manager = &inode_manager;
	this->disk = &storage;
}

FileAccessManager::~FileAccessManager() {}

/**
 * Returns the INode ID associated with a string path.
 * If the path cannot be found, 0 is returned.
 *
 * @param path: A NULL terminated sequence of characters.
 * @return The INode ID associated with the path, or 0 if not found.
 */
INode::ID FileAccessManager::getINodeFromPath(std::string path) {

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
		cur_inode_num = componentLookup(cur_inode_num, component);
		if (cur_inode_num == 0) {
			return 0;
		}
	}

	return componentLookup(cur_inode_num, path);
}

/**
 * Searches for a filename using a directory inode.
 * Search looks through all direct and indirect pointers
 * in the block.
 *
 * @param dir_inode_num: The INode ID of the directory.
 * @param filename: A string filename that is being searched for.
 * @return The INode ID of the filename if it is found, 0 otherwise.
 */
INode::ID FileAccessManager::componentLookup(INode::ID dir_inode_num, std::string filename) {
	// Read the directory inode
	INode dir_inode;
	this->inode_manager->get(dir_inode_num, dir_inode);

	// 1. Search directory's direct pointers
	for (size_t i = 0; i < INode::DIRECT_POINTERS; i++) {
		if (dir_inode.block_pointers[i] == 0) {
			return 0;
		}
		Block directory;
		this->disk->get(dir_inode.block_pointers[i], directory);

		// Check the directory for a record with matching name
		INode::ID inode_num = directLookup(&directory, filename);
		if (inode_num != 0) {
			return inode_num;
		}
	}

	// 2. Search single-indirect pointers
	for (size_t i = 0; i < INode::SINGLE_INDIRECT_POINTERS; i++) {
		if (dir_inode.block_pointers[INode::DIRECT_POINTERS + i] == 0) {
			return 0;
		}
		// Read in the data block containing directory pointers
		Block directory_ptrs_block;
		Block::ID *directory_ptrs = (Block::ID *) &directory_ptrs_block;
		this->disk->get(dir_inode.block_pointers[INode::DIRECT_POINTERS + i], directory_ptrs_block);

		// Search
		INode::ID inode_num = singleIndirectLookup(directory_ptrs, filename);
		if (inode_num != 0) {
			return inode_num;
		}
	}

	// 3. Search double-indirect pointers
	for (size_t i = 0; i < INode::DOUBLE_INDIRECT_POINTERS; i++) {
		if (dir_inode.block_pointers[INode::DIRECT_POINTERS + INode::SINGLE_INDIRECT_POINTERS + i] == 0) {
			return 0;
		}
		// Read in data block containing single-indirect pointers
		Block single_indirect_ptrs_block;
		Block::ID *single_indirect_ptrs = (Block::ID *) &single_indirect_ptrs_block;
		this->disk->get(dir_inode.block_pointers[INode::DIRECT_POINTERS + INode::SINGLE_INDIRECT_POINTERS + i], single_indirect_ptrs_block);

		// Search
		INode::ID inode_num = doubleIndirectLookup(single_indirect_ptrs, filename);
		if (inode_num != 0) {
			return inode_num;
		}
	}

	// 4. Search triple-indirect pointers
	for (size_t i = 0; i < INode::TRIPLE_INDIRECT_POINTERS; i++) {
		if (dir_inode.block_pointers[INode::DIRECT_POINTERS + INode::SINGLE_INDIRECT_POINTERS + INode::DOUBLE_INDIRECT_POINTERS + i] == 0) {
			return 0;
		}
		// Read in data block containing double-indirect pointers
		Block double_indirect_ptrs_block;
		Block::ID *double_indirect_ptrs = (Block::ID *) &double_indirect_ptrs_block;
		this->disk->get(dir_inode.block_pointers[INode::DIRECT_POINTERS + INode::SINGLE_INDIRECT_POINTERS + INode::DOUBLE_INDIRECT_POINTERS +i], double_indirect_ptrs_block);

		// Search
		INode::ID inode_num = tripleIndirectLookup(double_indirect_ptrs, filename);
		if (inode_num != 0) {
			return inode_num;
		}
	}

	// Couldn't find the filename in this directory-inode
	return 0;
}

/**
 * Searches n^3 directories for an entry with the given filename.
 *
 * @param double_indirect_ptrs: A pointer to a block containing double-indirect pointers.
 * @param filename: A string filename that is being searched for.
 * @return The INode ID of the filename if it is found, 0 otherwise.
 */
INode::ID FileAccessManager::tripleIndirectLookup(Block::ID *double_indirect_ptrs, std::string filename) {
	// Search each double-indirect pointer
	for (size_t i = 0; i < Block::SIZE / sizeof(Block::ID); i++) {

		// Don't check a NULL pointer
		if (double_indirect_ptrs[i] == 0) {
			continue;
		}

		// Load the block containing single-indirect pointers
		Block single_indirect_ptrs_block;
		Block::ID *single_indirect_ptrs = (Block::ID *) &single_indirect_ptrs_block;
		this->disk->get(double_indirect_ptrs[i], single_indirect_ptrs_block);
		INode::ID inode_num = doubleIndirectLookup(single_indirect_ptrs, filename);
		if (inode_num != 0) {
			return inode_num;
		}
	}

	// Couldn't find the filename in any of the directories
	return 0;
}

/**
 * Searches n^2 directories for an entry with the given filename.
 *
 * @param single_indirect_ptrs: A pointer to a block containing single-indirect pointers.
 * @param filename: A string filename that is being searched for.
 * @return The INode ID of the filename if it is found, 0 otherwise.
 */
INode::ID FileAccessManager::doubleIndirectLookup(Block::ID *single_indirect_ptrs, std::string filename) {
	// Search each indirect pointer
	for (size_t i = 0; i < Block::SIZE / sizeof(Block::ID); i++) {

		// Don't check a NULL pointer
		if (single_indirect_ptrs[i] == 0) {
			continue;
		}

		// Load the block containing directory pointers
		Block directory_ptrs_block;
		Block::ID *directory_ptrs = (Block::ID *) &directory_ptrs_block;
		this->disk->get(single_indirect_ptrs[i], directory_ptrs_block);
		INode::ID inode_num = singleIndirectLookup(directory_ptrs, filename);
		if (inode_num != 0) {
			return inode_num;
		}
	}

	// Couldn't find the filename in any of the directories
	return 0;
}

/**
 * Searches n directories for an entry with the given filename.
 *
 * @param directory_ptrs: A pointer to a block containing directory pointers.
 * @param filename: A string filename that is being searched for.
 * @return The INode ID of the filename if it is found, 0 otherwise.
 */
INode::ID FileAccessManager::singleIndirectLookup(Block::ID *directory_ptrs, std::string filename) {
	// For each pointer to a directory, load and check the directory
	for (size_t i = 0; i < Block::SIZE / sizeof(Block::ID); i++) {

		// Don't check a NULL pointer
		if (directory_ptrs[i] == 0) {
			continue;
		}

		// Load the directory
		Block directory;
		this->disk->get(directory_ptrs[i], directory);

		// Check the directory
		INode::ID inode_num = directLookup(&directory, filename);
		if (inode_num != 0) {
			return inode_num;
		}
	}

	// Couldn't find the filename in any of the directories
	return 0;
}

/**
 * Searches 1 block containing directory records for an entry with the given filename.
 *
 * @param block: A pointer to a block containing directory records.
 * @param filename: A string filename that is being searched for.
 * @return The INode ID of the filename if it is found, 0 otherwise.
 */
INode::ID FileAccessManager::directLookup(Block *block, std::string filename) {

	// Check block for the desired filename
	size_t offset = 0;
	while (offset < Block::SIZE) {
		DirectoryRecord *record = (DirectoryRecord *) (((char *) block) + offset);

		// Check if record is unused and shouldn't be checked
		if (record->inode_ID == 0) {
			// Make sure that this isn't the last used entry in the block
			if (offset + record->length == offset) {
				return 0;
			}
			offset += record->length;
			continue;
		}

		// Record is being used - should be checked
		if (record->name == filename) {
			return record->inode_ID;
		}

		// Not the correct filename, check next entry
		offset += record->length;
	}

	// Didn't find filename in this directory
	return 0;
}



// ---------------------------------------------
// TODO Functions
// ---------------------------------------------
// Read data from a file
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
	uint64_t file_offset_block_n = offset/Block::SIZE;

	// calculate start byte in block for I/O
	fileBlockInfo.byte_offset = offset % Block::SIZE;

	// calculate number of bytes to copy to user
	uint64_t total_block_n = (inode.size - 1) / Block::SIZE;
	if (total_block_n > file_offset_block_n) {
		fileBlockInfo.bytes_block_io = Block::SIZE - fileBlockInfo.byte_offset;
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
		current_offset_block_n -= Block::SIZE * DIRECT_BLOCKS_COUNT;
		break;
	case 1:
		current_offset_block_n -= Block::SIZE * INDIRECT_REF_COUNT;
		break;
	case 2:
		current_offset_block_n -= Block::SIZE * INDIRECT_REF_COUNT * INDIRECT_REF_COUNT;
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
	uint64_t double_indirect_block_offset = single_indirect_block_offset + SINGLE_INDIRECT_SIZE;
	uint64_t triple_indirect_block_offset = double_indirect_block_offset + DOUBLE_INDIRECT_SIZE;
	uint64_t max_file_offset = triple_indirect_block_offset + TRIPLE_INDIRECT_SIZE;

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
