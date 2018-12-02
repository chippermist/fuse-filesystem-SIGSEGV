#include "INode.h"

#include <cstring>
#include <ctime>
#include <fuse.h>

INode::INode() {
  std::memset(this, 0, sizeof(INode));
}

INode::INode(FileType type, uint16_t mode, uint64_t dev): INode() {
  fuse_context* context = fuse_get_context();

  this->type   = type;
  this->mode   = mode & ~context->umask;
  this->uid    = context->uid;
  this->gid    = context->gid;
  this->atime  = time(NULL);
  this->ctime  = this->atime;
  this->mtime  = this->atime;
  this->links  = 1;
  this->dev    = dev; // Huh?
}
