#include "INode.h"

#include <cstring>
#include <ctime>

INode::INode() {
  std::memset(this, 0, sizeof(INode));
}

INode::INode(FileType type, uint16_t mode, uint64_t dev): INode() {
  this->type   = type;
  this->mode   = mode;
  this->atime  = time(NULL);
  this->ctime  = this->atime;
  this->mtime  = this->atime;
  this->links  = 1;
  this->dev    = dev; // Huh?
}
