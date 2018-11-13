#pragma once

#include "INode.h"

class INodeManager {
public:
  virtual INode::ID reserve() = 0;
  virtual void release(INode::ID id) = 0;
  virtual void get(INode::ID inode_num, INode& user_inode) = 0;
  virtual void set(INode::ID inode_num, INode& user_inode) = 0;
};
