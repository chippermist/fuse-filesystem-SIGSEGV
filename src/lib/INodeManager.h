#ifndef SIGSEGV_INODEMANAGER_H
#define SIGSEGV_INODEMANAGER_H

#include "INode.h"

class INodeManager {
public:
  virtual void mkfs() = 0;
  virtual INode::ID reserve() = 0;
  virtual void release(INode::ID id) = 0;
};

#endif
