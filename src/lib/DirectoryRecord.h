#pragma once

#include <cstdint>
#include "INode.h"

// enum FileTypeDirectory: uint8_t {
//   FREE = 0,
//   REGULAR = 1,
//   DIRECTORY = 2,
//   SYMLINK = 3
// };

struct DirectoryRecord {
  uint16_t length;
  INode::ID inode_ID;
  char name[];
  // uint8_t type;  // was added to specify type
};
