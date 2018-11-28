#pragma once

#include "INode.h"

#include <map>
#include <string>
#include <vector>

class Directory {
private:
  INode::ID inode_id;
  std::map<std::string, INode::ID> entries;

public:
  Directory(INode::ID id, INode::ID parent);
  Directory(INode::ID id, const char* buffer, size_t size);

  INode::ID id() const;
  void insert(const std::string& name, INode::ID id);
  void remove(const std::string& name);
  INode::ID search(const std::string& name) const;
  std::vector<char> serialize() const;
};
