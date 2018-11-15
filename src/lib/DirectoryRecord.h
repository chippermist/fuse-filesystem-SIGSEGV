#pragma once

#include <cstdint>
#include "INode.h"

struct DirectoryRecord {
  uint16_t length;
  INode::ID inode_ID;
  char name[];
};
