#include "Directory.h"

#include <cstring>

// To be used when creating (mkdiring) a new directory:
Directory::Directory(INode::ID id, INode::ID parent): inode_id(id), entries() {
  insert(".",  id);
  insert("..", parent);
}

// To be used when loading an existing directory from disk:
Directory::Directory(INode::ID id, const char* data, size_t size): inode_id(id), entries() {
  size_t index = 0;
  while(index < size) {
    INode::ID eid;
    std::memcpy(&eid, &data[index], sizeof(INode::ID));
    std::string name(&data[index + sizeof(INode::ID)]);
    index += name.length() + sizeof(INode::ID) + 1;
    insert(name, eid);
  }
}

INode::ID Directory::id() const {
  return inode_id;
}

void Directory::insert(const std::string& name, INode::ID id) {
  entries[name] = id;
}

void Directory::remove(const std::string& name) {
  entries.erase(name);
}

INode::ID Directory::search(const std::string& name) const {
  auto itr = entries.find(name);
  return (itr == entries.end())? 0 : itr->second;
}

std::vector<char> Directory::serialize() const {
  int bytes = 0;
  for(const auto itr: entries) {
    bytes += itr.first.length() + sizeof(INode::ID) + 1;
  }

  std::vector<char> data;
  data.resize(bytes);

  char* tgt = &data[0];
  for(const auto itr: entries) {
    std::memcpy(tgt, &itr.second, sizeof(INode::ID));
    tgt += sizeof(INode::ID);
    std::strcpy(tgt, itr.first.c_str());
    tgt += itr.first.length() + 1;
  }

  return data;
}

std::unordered_map<std::string, INode::ID> Directory::contents() {
  return entries;
}
