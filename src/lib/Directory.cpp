#include "Directory.h"

#include <cstring>

// To be used when creating (mkdiring) a new directory:
Directory::Directory(INode::ID id, INode::ID parent): mID(id), mEntries() {
  insert(".",  id);
  insert("..", parent);
}

// To be used when loading an existing directory from disk:
Directory::Directory(INode::ID id, const char* data, size_t size): mID(id), mEntries() {
  size_t index = 0;
  while(index < size) {
    INode::ID eid;
    std::memcpy(&eid, &data[index], sizeof(INode::ID));
    std::string name(&data[index + sizeof(INode::ID)]);
    index += name.length() + sizeof(INode::ID) + 1;
    insert(name, eid);
  }
}

bool Directory::contains(const std::string& name) const {
  return mEntries.find(name) != mEntries.end();
}

const std::unordered_map<std::string, INode::ID>& Directory::entries() const {
  return mEntries;
}

INode::ID Directory::id() const {
  return mID;
}

void Directory::insert(const std::string& name, INode::ID id) {
  mEntries[name] = id;
}

bool Directory::isEmpty() const {
  return mEntries.size() < 3;
}

void Directory::remove(const std::string& name) {
  mEntries.erase(name);
}

INode::ID Directory::search(const std::string& name) const {
  auto itr = mEntries.find(name);
  return (itr == mEntries.end())? 0 : itr->second;
}

std::vector<char> Directory::serialize() const {
  int bytes = 0;
  for(const auto itr: mEntries) {
    bytes += itr.first.length() + sizeof(INode::ID) + 1;
  }

  std::vector<char> data;
  data.resize(bytes);

  char* tgt = &data[0];
  for(const auto itr: mEntries) {
    std::memcpy(tgt, &itr.second, sizeof(INode::ID));
    tgt += sizeof(INode::ID);
    std::strcpy(tgt, itr.first.c_str());
    tgt += itr.first.length() + 1;
  }

  return data;
}

std::unordered_map<std::string, INode::ID> Directory::contents() {
  return mEntries;
}
