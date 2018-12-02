#pragma once

#include <cstring>
#include <stdexcept>
#include <cassert>
#include "../Superblock.h"
#include "../INodeManager.h"
#include "../Storage.h"
#include "../Block.h"

class ListINodeManager: public INodeManager {
public:
    ListINodeManager(Storage& storage);
    ~ListINodeManager();
    
    virtual void mkfs();
    INode::ID reserve();
    void release(INode::ID id);
    void get(INode::ID id, INode& dst);
    void set(INode::ID id, const INode& src);
    INode::ID getRoot();
    
    void update_superblock();
    
private:
    Block::ID top_inodeBlk;
    uint64_t top_index;
    uint64_t last_index;
    Block::ID first_inodeBlk;
    Block::ID last_inodeBlk;

    static const uint64_t root = 1;
    uint64_t num_inodes;
    Storage *disk;
};
