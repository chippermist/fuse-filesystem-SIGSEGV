#pragma once

#include "INode.h"

#include <map>
#include <string>

class Directory {
public:
  static Directory get(INode::ID id);
  static Directory get(const std::string& path);

private:
  INode::ID id;
  std::map<std::string, INode::ID> entries;

public:
  void insert(const std::string& name, INode::ID id);
  void remove(const std::string& name);
  void save();
  INode::ID search(const std::string& name) const;
};
