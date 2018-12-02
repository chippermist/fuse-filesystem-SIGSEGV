#include "ListINodeManager.h"

namespace {
    struct freeINodeBlock {
        static const int NREFS = (Block::SIZE / sizeof(INode::ID));
        INode::ID free_inodes[NREFS];
    };
    
    struct Config {
        Block::ID top_inodeBlk;
        uint64_t top_index;
        uint64_t last_index;
        Block::ID first_inodeBlk;
        Block::ID last_inodeBlk;
    };
}

ListINodeManager::ListINodeManager(Storage& storage): disk(&storage) {
    Block block;
    Superblock* superblock = (Superblock*) &block;
    Config* config = (Config*) superblock->inode_config;
    this->disk->get(0, block);
    
    assert(Block::SIZE % INode::SIZE == 0);
    
    this->num_inodes = (Block::SIZE / INode::SIZE) * superblock->inode_block_count;

    this->top_inodeBlk   = config->top_inodeBlk;
    this->top_index   = config->top_index;
    this->last_index  = config->last_index;
    this->last_inodeBlk  = config->last_inodeBlk;
    this->first_inodeBlk = config->first_inodeBlk;
}

ListINodeManager::~ListINodeManager() {}

// Initialize inodes during mkfs()
void ListINodeManager::mkfs() {
    
    // TODO: Initialize root directory and set this->root
    
    // Read superblock
    Block superblock_blk;
    this->disk->get(0, superblock_blk);
    Superblock* superblock = (Superblock*) &superblock_blk;
    Config* config = (Config*) superblock->inode_config;
    Block::ID start = superblock->inode_block_start;
    uint64_t count  = superblock->inode_block_count;
    uint64_t num_inodes_per_block = Block::SIZE / INode::SIZE;
    
    
    Block block_for_inodes;
    uint64_t inodeID = -1;
    Block::ID inodeBlk = start;
    
    Block block_free_inodes;
    freeINodeBlock *free_inodeIDs = (freeINodeBlock *) &block_free_inodes;
    Block::ID free_inodeBlk = start + count - 1;
    int perBlockFreeInodeCount = freeINodeBlock::NREFS - 1;
    
    while (inodeBlk < free_inodeBlk) {
        // Read in the inode block
        this->disk->get(inodeBlk, block_for_inodes);
        
        // Get block from end of IList to set free inodes
        if (inodeBlk == start) {
            this->disk->get(free_inodeBlk, block_for_inodes);
        }
        
        // Zero out each inode in the block except INode 0 of block 0
        for (uint64_t inode_index = 0; inode_index < num_inodes_per_block; inode_index++) {
            if (inodeBlk == start && inode_index == 0) {
                memset(&(block_for_inodes.data[inode_index * INode::SIZE]), FileType::REGULAR, INode::SIZE);
                inodeID++;
            } else {
                memset(&(block_for_inodes.data[inode_index * INode::SIZE]), FileType::FREE, INode::SIZE);
                inodeID++;
                
                // register the free inode in inode_free_list
                free_inodeIDs->free_inodes[perBlockFreeInodeCount] = inodeID;
                perBlockFreeInodeCount--;
                // if free inode block is full, set free_block
                if (perBlockFreeInodeCount < 0) {
                    this->disk->set(free_inodeBlk, block_free_inodes);
                    free_inodeBlk--;
                    
                    // allocate next block for free inodes only if it doesnt overflow to already allocated inode_block
                    if (free_inodeBlk == inodeBlk) {
                        break;
                    }
                    this->disk->get(free_inodeBlk, block_for_inodes);
                    perBlockFreeInodeCount = freeINodeBlock::NREFS - 1;
                }
            }
        }
        this->disk->set(inodeBlk, block_for_inodes);
        inodeBlk++;
    }
    
    // At this point, inodeBlk is colliding with free inode i.e. (inodeBlk == free_inodeBlk)
    // Either inode_block is overflowing into inode_free_block
    // Or inode_free_block is overflowing into inode_block (In this case we dont allow allocation of last inode block used)
    if (perBlockFreeInodeCount != freeINodeBlock::NREFS - 1) {
        // Write last free inode list block
        this->disk->set(free_inodeBlk, block_free_inodes);
        config->last_index = perBlockFreeInodeCount + 1;
        config->last_inodeBlk = free_inodeBlk;
    } else {
        // If we just started looking at this free block,
        // the real last free block was before this one.
        config->last_index = 0;
        config->last_inodeBlk = free_inodeBlk + 1;
    }
    
    // Update superblock
    config->top_inodeBlk = start + count - 1;
    config->top_index = freeINodeBlock::NREFS - 1;
    config->first_inodeBlk = start + count - 1;
    superblock->inode_block_count = inodeBlk - start;  // we don't allow allocation of last inode block
    this->disk->set(0, superblock_blk);
    
    // Update class members with true values
    this->num_inodes = (Block::SIZE / INode::SIZE) * superblock->inode_block_count;
    this->top_inodeBlk   = config->top_inodeBlk;
    this->top_index   = config->top_index;
    this->last_index  = config->last_index;
    this->last_inodeBlk  = config->last_inodeBlk;
    this->first_inodeBlk = config->first_inodeBlk;
    
    return;
}

void ListINodeManager::update_superblock() {
    Block block;
    Superblock* superblock = (Superblock*) &block;
    Config* config = (Config*) superblock->inode_config;
    
    this->disk->get(0, block);
    config->top_inodeBlk = this->top_inodeBlk;
    config->top_index = this->top_index;
    this->disk->set(0, block);
}


// Get an inode from the freelist and return it
INode::ID ListINodeManager::reserve() {
    // Check if free list is almost empty and refuse allocation of last inode
    if (this->top_inodeBlk == this->last_inodeBlk && this->top_index < this->last_index) {
        throw std::out_of_range("Can't get any more free inodes - free list is empty!");
    }
    
    // Get next free inode
    Block block;
    freeINodeBlock *node = (freeINodeBlock *) &block;
    this->disk->get(this->top_inodeBlk, block);
    INode::ID free_inode_num = node->free_inodes[this->top_index];
    if (this->top_index == 0) {
        this->top_index = freeINodeBlock::NREFS - 1;
        this->top_inodeBlk--;
    } else {
        this->top_index--;
    }
    
    this->update_superblock();
    return free_inode_num;
}

// Free an inode and return to the freelist
void ListINodeManager::release(INode::ID inode_num) {
    
    // Check if valid id
    if (inode_num >= this->num_inodes || inode_num < this->root) {
        throw std::out_of_range("INode index is out of range!");
    }
    
    uint64_t num_inodes_per_block = (Block::SIZE / INode::SIZE);
    uint64_t block_index = inode_num / num_inodes_per_block;
    uint64_t inode_index = inode_num % num_inodes_per_block;
    
    // Load the inode and modify attribute
    Block block;
    this->disk->get(1 + block_index, block);
    INode *inode = (INode *) &(block.data[inode_index * INode::SIZE]);
    inode->type = FileType::FREE;
    
    // Write the inode back to disk
    this->disk->set(1 + block_index, block);
    
    // If insertion causes index to overflow in the free block,
    // move to previous block in free list.
    if (this->top_index == freeINodeBlock::NREFS - 1) {
        if (this->top_inodeBlk == this->first_inodeBlk) {
            throw std::out_of_range("Can't insert block at top of inode free list!");
        }
        this->top_inodeBlk = this->top_inodeBlk + 1;
        this->top_index = 0;
    } else {
        this->top_index++;
    }

    
    // Update the free list
    freeINodeBlock *node = (freeINodeBlock *) &block;
    this->disk->get(this->top_inodeBlk, block);
    node->free_inodes[this->top_index] = inode_num;
    this->disk->set(this->top_inodeBlk, block);
    this->update_superblock();
}

// Reads an inode from disk into the memory provided by the user
void ListINodeManager::get(INode::ID inode_num, INode& user_inode) {
    
    // Check if valid ID
    if (inode_num >= this->num_inodes || inode_num < this->root) {
        throw std::out_of_range("INode index is out of range!");
    }
    
    uint64_t num_inodes_per_block = (Block::SIZE / INode::SIZE);
    uint64_t block_index = inode_num / num_inodes_per_block;
    uint64_t inode_index = inode_num % num_inodes_per_block;
    
    Block block;
    this->disk->get(1 + block_index, block);
    INode *inode = (INode *) &(block.data[inode_index * INode::SIZE]);
    
    memcpy(&user_inode, inode, INode::SIZE);
}

void ListINodeManager::set(INode::ID inode_num, const INode& user_inode) {
    
    // Check if valid ID
    if (inode_num >= this->num_inodes || inode_num < this->root) {
        throw std::out_of_range("INode index is out of range!");
    }
    
    uint64_t num_inodes_per_block = (Block::SIZE / INode::SIZE);
    uint64_t block_index = inode_num / num_inodes_per_block;
    uint64_t inode_index = inode_num % num_inodes_per_block;
    
    Block block;
    this->disk->get(1 + block_index, block);
    INode *inode = (INode *) &(block.data[inode_index * INode::SIZE]);
    
    memcpy(inode, &user_inode, INode::SIZE);
    this->disk->set(1 + block_index, block);
}

INode::ID ListINodeManager::getRoot() {
    return this->root;
}
