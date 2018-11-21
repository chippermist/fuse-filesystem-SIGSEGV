#include "Directory.h"

Directory Directory::get(INode::ID id) {
  INode inode = INode::get(id);
  char* data = new char[inode.size];
  inode.read(data, inode.size);

  int index = 0;
  Directory directory;
  while(index < inode.size) {
    INode::ID eid;
    std::memcpy(&eid, &data[index], sizeof(INode::ID));
    std::string name(&data[index + sizeof(INode::ID)]);
    index += name.length() + sizeof(INode::ID) + 1;
    directory.insert(name, eid);
  }

  return directory;
}

Directory Directory::get(const std::string& path) {
  return Directory::get(INode::id(path));
}

void Directory::insert(const std::string& name, INode::ID id) {
  entries[name] = id;
}

Directory Directory::mkdir(INode::ID parent) {
  return Directory::mkdir(parent, INode::reserve());
}

Directory Directory::mkdir(INode::ID parent, INode::ID id) {
  Directory directory;
  directory.insert(".",  id);
  directory.insert("..", parent);
  return directory;
}

void Directory::remove(const std::string& name) {
  entries.erase(name);
}

void Directory::save() {
  int bytes = 0;
  for(const auto itr: entries) {
    bytes += itr.first.length() + sizeof(INode::ID) + 1;
  }

  int index = 0;
  char* data = new char[bytes];
  for(const auto itr: entries) {
    std::memcpy(&data[index], &itr.second, sizeof(INode::ID));
    std::strcpy(&data[index + sizeof(INode::ID)], itr.first.c_str());
    index += itr.first.length() + sizeof(INode::ID) + 1;
  }

  INode inode = INode::get(id);
  inode.write(data);
  delete [] data;

  if(inode.size > bytes) {
    inode.truncate(bytes);
  }
}

INode::ID Directory::search(const std::string& name) const {
  auto itr = entries.find(name);
  return (itr == entries.end())? 0 : itr->second;
}
