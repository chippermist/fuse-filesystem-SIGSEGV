#pragma once

#include "INode.h"

#if defined(__linux__)
  #include <sys/statvfs.h>
#else
  #include <fuse.h>
#endif

struct statvfs;

class INodeManager {
public:
  virtual ~INodeManager() {}

  virtual void mkfs() = 0;
  virtual void statfs(struct statvfs* info) = 0;
  virtual INode::ID getRoot() = 0;

  virtual void get(INode::ID id, INode& dst) = 0;
  virtual void set(INode::ID id, const INode& src) = 0;

  virtual INode::ID reserve() = 0;
  virtual void release(INode::ID id) = 0;
};
