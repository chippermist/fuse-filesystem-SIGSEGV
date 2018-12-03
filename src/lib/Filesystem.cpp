#include "Filesystem.h"
#include "FSExceptions.h"
#include "Superblock.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <stack>
#include <stdexcept>
#include <cstdlib>
#include <string>
#include <unistd.h>

#if defined(__linux__)
  #include <sys/statfs.h>
  #include <sys/vfs.h>
  #include <sys/statvfs.h>
#endif

#include <fuse.h>


Filesystem::Filesystem(BlockManager& block_manager, INodeManager& inode_manager) {
  this->block_manager = &block_manager;
  this->inode_manager = &inode_manager;

  uint64_t ipb   = Block::SIZE / sizeof(Block::ID);
  max_file_size  = INode::DIRECT_POINTERS;
  max_file_size += INode::SINGLE_INDIRECT_POINTERS * ipb;
  max_file_size += INode::DOUBLE_INDIRECT_POINTERS * ipb * ipb;
  max_file_size += INode::TRIPLE_INDIRECT_POINTERS * ipb * ipb * ipb;
  max_file_size *= Block::SIZE;
}

Filesystem::~Filesystem() {
  // Nothing to do.
}

void Filesystem::mkfs(uint64_t nblocks, uint64_t niblocks) {
  Block block;
  Superblock* superblock = (Superblock*) block.data;
  std::memset(block.data, 0, Block::SIZE);

  superblock->magic       = 3199905246;
  superblock->block_size  = Block::SIZE;
  superblock->block_count = nblocks;

  superblock->inode_block_start = 1;
  superblock->inode_block_count = niblocks;
  superblock->data_block_start  = niblocks + 1;
  superblock->data_block_count  = nblocks - niblocks - 1;

  block_manager->setSuperblock(block);
  inode_manager->mkfs();
  block_manager->mkfs();

  INode::ID id = inode_manager->getRoot();
  INode inode(FileType::DIRECTORY, 0777);
  if(uid_t uid = getuid()) {
    inode.uid = uid;
  }
  if(gid_t gid = getgid()) {
    inode.gid = gid;
  }
  inode.links = 2;
  save(id, inode);

  Directory root(id, id);
  save(root);
}

void Filesystem::statfs(struct statvfs* info) {
  Block block;
  Superblock* superblock = (Superblock*) block.data;
  block_manager->getSuperblock(block);

  // Based on http://pubs.opengroup.org/onlinepubs/009604599/basedefs/sys/statvfs.h.html
  // Also see http://man7.org/linux/man-pages/man3/statvfs.3.html
  info->f_bsize   = superblock->block_size; // File system block size.
  info->f_frsize  = superblock->block_size; // Fundamental file system block size.
  info->f_fsid    = superblock->magic;      // File system ID.
  info->f_flag    = 0;                      // Bit mask of f_flag values.
  info->f_namemax = 256;                    // Maximum filename length.
}

int Filesystem::mount(char* program, fuse_operations* ops) {
  if(mount_point == NULL) {
    std::cerr << "No mount point given.\n";
    exit(1);
  }

  char s[] = "-s"; // Use a single thread.
  char d[] = "-d"; // Print debuging output.
  char f[] = "-f"; // Run in the foreground.
  char o[] = "-o"; // Other options
  char p[] = "default_permissions"; // Defer permissions checks to kernel
  char r[] = "allow_other"; // Allow all users to access files

  int argc = 0;
  char* argv[12] = {0};

  argv[argc++] = program;
  if(!parallel) argv[argc++] = s;
  if(debug)     argv[argc++] = d;
  argv[argc++] = f;
  argv[argc++] = mount_point;
  argv[argc++] = o;
  argv[argc++] = p;
  argv[argc++] = o;
  argv[argc++] = r;

  return fuse_main(argc, argv, ops, 0);
}

Directory Filesystem::getDirectory(INode::ID id) {
  INode inode;
  inode_manager->get(id, inode);
  if(inode.type != FileType::DIRECTORY) {
    throw NotADirectory();
  }

  char* buffer = new char[inode.size];
  read(id, buffer, inode.size, 0);

  Directory directory(id, buffer, inode.size);
  delete [] buffer;
  return directory;
}

Directory Filesystem::getDirectory(const std::string& path) {
  return getDirectory(getINodeID(path));
}

INode Filesystem::getINode(INode::ID id) {
  if(id == 0) throw NoSuchEntry();

  INode inode;
  inode_manager->get(id, inode);
  return inode;
}

INode Filesystem::getINode(const std::string& path) {
  return getINode(getINodeID(path));
}

INode::ID Filesystem::getINodeID(const std::string& path) {
  INode::ID id = inode_manager->getRoot();
  if(path == "/") return id;

  size_t start = 1;
  size_t found = 0;

  while(found != std::string::npos) {
    found = path.find('/', start);
    std::string component = path.substr(start, found - start);
    start = found + 1;

    Directory dir = getDirectory(id);
    id = dir.search(component);
  }

  return id;
}

INode::ID Filesystem::getINodeID(const char* path, fuse_file_info* info) {
  return (info->fh != 0) ? info->fh : getINodeID(path);
}

INode::ID Filesystem::newINodeID() {
  return inode_manager->reserve();
}

void Filesystem::save(const Directory& directory) {
  std::vector<char> data = directory.serialize();
  write(directory.id(), &data[0], data.size(), 0, true);
}

void Filesystem::save(INode::ID id, const INode& inode) {
  inode_manager->set(id, inode);
}

int Filesystem::read(INode::ID file_inode_num, char *buf, uint64_t size, uint64_t offset) {
  // Read the file's inode and do some sanity checks
  INode file_inode = getINode(file_inode_num);
  if (offset >= file_inode.size) {
    return 0; // Can't begin reading from after file
  }

  file_inode.atime = time(NULL);

  // Only read until the end of the file
  if (offset + size > file_inode.size) {
    size = file_inode.size - offset;
  }

  size_t total_read = 0;
  while (size > 0) {

    // Get datablock of current offset
    Block::ID cur_block_id = blockAt(file_inode, offset);
    Block block;
    this->block_manager->get(cur_block_id, block);

    /*
      How many bytes to read?
      a) Normally read whole blocks at a time.
      b) If the offset isn't block aligned though
         (which could happen for the first block),
         then only read until the end of the block.
      c) Don't read past the end of the file.
      d) Of course, don't read more than what was asked!
    */
    size_t to_read = Block::SIZE - (offset % Block::SIZE);

    if (offset + to_read > file_inode.size) {
      to_read = file_inode.size - offset;
    }

    if (to_read > size) {
      to_read = size;
    }

    // Copy data from block into buffer
    memcpy(buf, block.data + (offset % Block::SIZE), to_read);

    // Update offset, buf pointer, and num bytes left to read
    offset += to_read;
    buf += to_read;
    size -= to_read;

    total_read += to_read;
  }
  // relatime - Don't write back changes to file_inode!
  // this->inode_manager->set(file_inode_num, file_inode);
  return total_read;
}

Block::ID Filesystem::blockAt(const INode& inode, uint64_t offset) {
  // This function only gets existing blocks.
  // It might need reworking for writes.
  if (offset >= inode.size) {
    throw std::out_of_range("Offset greater than file size.");
  }

  if (offset < INode::DIRECT_POINTERS * Block::SIZE) {
    return inode.block_pointers[offset / Block::SIZE];
  }

  uint64_t size  = Block::SIZE;
  uint64_t scale = Block::SIZE / sizeof(Block::ID);
  offset -= INode::DIRECT_POINTERS * Block::SIZE;
  for (int i = 0; i < 3; ++i) {
    if (offset < size * scale) {
      Block::ID bid = inode.block_pointers[INode::DIRECT_POINTERS + i];
      return indirectBlockAt(bid, offset, size);
    }

    size   *= scale;
    offset -= size;
  }

  // If we get here, something has gone very wrong.
  throw std::out_of_range("Offset greater than maximum file size!");
}

Block::ID Filesystem::indirectBlockAt(Block::ID bid, uint64_t offset, uint64_t size) {
  Block block;
  this->block_manager->get(bid, block);
  Block::ID* refs = (Block::ID*) &block;

  uint64_t index = offset / size;
  if(size == Block::SIZE) {
    return refs[index];
  }

  uint64_t scale = Block::SIZE / sizeof(Block::ID);
  return indirectBlockAt(refs[index], offset % size, size / scale);
}

int Filesystem::truncate(INode::ID id, uint64_t length) {
  write(id, NULL, 0, length, true);
  return 0;
}

std::string Filesystem::dirname(const char* path_cstring) {
  std::string path(path_cstring);
  // Remove all repeated /'s
  path.erase(std::unique(path.begin(), path.end(), [](char &a, char &b) {
    return a == '/' && b == '/';
  }), path.end());

  std::stack<std::string> folder_names;

  for(size_t i=0; i<path.size(); ++i) {
    if(i == 0 && path[i] == '/') {
      continue;
    }

    std::string str;
    while(path[i] != '/' && i < path.length()) {
      str += path[i++];
    }
    //cout << str << endl;
    if(str == ".") {
      continue;
    }
    else if (str == ".." && !folder_names.empty()) {
      folder_names.pop();
      continue;
    }
    else if (str == "..") {
      continue;
    }
    folder_names.push(str);
  }
  folder_names.pop(); // Removing the filename

  std::string dir_path;
  while(!folder_names.empty()) {
    dir_path = "/" + folder_names.top() + dir_path;
    folder_names.pop();
  }
  if (dir_path.length() == 0) {
    dir_path = "/";
  }
  return dir_path;
}

std::string Filesystem::basename(const char* path_cstring) {
  std::string path(path_cstring);
  size_t loc = path.find_last_of('/');
  if (loc != std::string::npos) {
    return (path.substr(loc + 1));
  }
  return "";
}

void Filesystem::unlink(INode::ID id) {
  INode inode = getINode(id);
  if(inode.links < 2) {
    truncate(id, 0);
    inode_manager->release(id);
  }
  else {
    inode.ctime = time(NULL);
    inode.links -= 1;
    save(id, inode);
  }
}
