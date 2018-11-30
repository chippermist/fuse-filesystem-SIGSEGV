#pragma once

#include "INode.h"

#include <unordered_map>
#include <string>
#include <vector>

class Directory {
private:
  INode::ID mID;
  std::unordered_map<std::string, INode::ID> mEntries;

public:
  Directory(INode::ID id, INode::ID parent);
  Directory(INode::ID id, const char* buffer, size_t size);

  INode::ID id() const;
  const std::unordered_map<std::string, INode::ID>& entries() const;

  bool contains(const std::string& name) const;
  void insert(const std::string& name, INode::ID id);
  bool isEmpty() const;
  void remove(const std::string& name);
  INode::ID search(const std::string& name) const;
  std::vector<char> serialize() const;
  std::unordered_map<std::string, INode::ID> contents();
};
