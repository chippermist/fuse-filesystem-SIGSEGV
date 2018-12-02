#include "INode.h"

// INode::INode(FileType type, uint16_t mode, uint64_t dev): INode() {
//   this->type   = type;
//   this->mode   = mode;
//   this->atime  = time(NULL);
//   this->ctime  = this->atime;
//   this->mtime  = this->atime;
//   this->links  = 1;
//   this->dev    = dev; // Huh?
// }

// INode::INode() {
//   core = nullptr;
// }

INode(Core* core): core(core) {
  reference();
}

INode::INode(INode&& other) {
  core = other.core;
  other.core = nullptr;
}

INode::INode(const INode& other) {
  other.reference();
  core = other.core;
}

INode::~INode() {
  dereference();
}

void INode::dereference() {
  if(isNull()) return;
  core->refs += 1;
}

INode::ID INode::id() const {
  return core->id;
}

bool INode::isNull() const {
  return core == nullptr;
}

void INode::reference() {
  if(isNull()) return;
  if(core->refs -= 1 == 0) {
    delete core;
  }
}

void INode::read(std::function<void(const INode::Data&)> callback) {
  // ReadLock lock(core);
  callback(*core->data);
}

void INode::save(bool force) {
  if(core->dirty | force) {
    core->manager->save(*this);
    core->dirty = false;
  }
}

void INode::save(std::function<void(INode::Data&)> callback) {
  // WriteLock lock(core);
  callback(*core->data);

  core->manager->save(*this);
  core->dirty = false;
}

void INode::write(std::function<void(INode::Data&)> callback) {
  // WriteLock lock(core);
  callback(*core->data);
  core->dirty = true;
}

INode& INode::operator = (INode&& other) {
  std::swap(core, other.core);
  return *this;
}

INode& INode::operator = (const INode& other) {
  other.reference();
  dereference();

  core = other.core;
  return *this;
}
