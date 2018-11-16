#pragma once

#include "INode.h"

class INodeManager {
public:
  virtual ~INodeManager() {}
  virtual INode::ID reserve() = 0;
  virtual void release(INode::ID id) = 0;
  virtual void get(INode::ID id, INode& dst) = 0;
  virtual void set(INode::ID id, const INode& src) = 0;
  virtual INode::ID getRoot() = 0;
  virtual void mkfs() = 0;
};
