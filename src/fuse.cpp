#include "lib/Filesystem.h"
#include "lib/storage/MemoryStorage.h"
#include "lib/storage/FileStorage.h"
#include "lib/inodes/LinearINodeManager.h"
#include "lib/blocks/StackBasedBlockManager.h"

#if defined(__linux__)
  #include <sys/statfs.h>
  #include <sys/vfs.h>
#endif

#include <fuse.h>
#include <cstring>
#include <cinttypes>

#ifndef NDEBUG
  #include <cstdio>
  #define debug(format, ...) fprintf(stderr, format, __VA_ARGS__)
#else
  #define debug(...)
#endif

#define UNUSED(x) ((void) (x))

// Global Variables
INodeManager *inode_manager;
Filesystem *fs;

extern "C" {

  // Declarations to resolve linker errors
  int   fs_chmod(const char*, mode_t);
  int   fs_chown(const char*, uid_t, gid_t);
  int   fs_flush(const char*, fuse_file_info*);
  int   fs_fsync(const char*, int, fuse_file_info*);
  int   fs_getattr(const char*, struct stat*);
  int   fs_getxattr(const char*, const char*, char*, size_t);
  void* fs_init(struct fuse_conn_info *);
  int   fs_link(const char*, const char*);
  int   fs_listxattr(const char*, char*, size_t);
  int   fs_mkdir(const char*, mode_t);
  int   fs_mknod(const char*, mode_t, dev_t);
  int   fs_open(const char*, fuse_file_info*);
  int   fs_read(const char*, char*, size_t, off_t, fuse_file_info*);
  int   fs_readdir(const char*, void*, fuse_fill_dir_t, off_t, struct fuse_file_info*);
  int   fs_readlink(const char*, char*, size_t);
  int   fs_release(const char*, fuse_file_info*);
  int   fs_removexattr(const char*, const char*);
  int   fs_rename(const char*, const char*);
  int   fs_rmdir(const char*);
  int   fs_setxattr(const char*, const char*, const char*, size_t, int);
  int   fs_statfs(const char*, struct statvfs*);
  int   fs_symlink(const char*, const char*);
  int   fs_truncate(const char*, off_t);
  int   fs_unlink(const char*);
  int   fs_utime(const char*, utimbuf*);
  int   fs_write(const char*, const char*, size_t, off_t, fuse_file_info*);


  int fs_chmod(const char* path, mode_t mode) {
    debug("chmod       %s to %03o\n", path, mode);

    // Check if path exists
    INode::ID inode_id = fs->getINodeID(path);
    if (inode_id == 0) return -1;

    // Update INode
    INode inode = fs->getINode(inode_id);
    inode.ctime = time(NULL);
    inode.mode  = mode;
    fs->save(inode_id, inode);
    return 0;
  }

  int fs_chown(const char* path, uid_t uid, gid_t gid) {
    debug("chown       %s to %d:%d\n", path, uid, gid);

    // Check if path exists
    INode::ID inode_id = fs->getINodeID(path);
    if (inode_id == 0) return -1;

    // Update INode
    INode inode = fs->getINode(inode_id);
    inode.ctime = time(NULL);
    inode.uid   = uid;
    inode.gid   = gid;
    fs->save(inode_id, inode);
    return 0;
  }

  int fs_flush(const char* path, fuse_file_info* info) {
    debug("flush       %s\n", path);
    UNUSED(info);

    // TODO...
    return 0;
  }

  int fs_fsync(const char* path, int unknown, fuse_file_info* info) {
    debug("fsync       %s\n", path);
    UNUSED(unknown);
    UNUSED(info);

    // TODO...
    return 0;
  }

  int fs_getattr(const char* path, struct stat* info) {
    debug("getattr     %s\n", path);
    UNUSED(info);

    // Check if path exists
    INode::ID inode_id = fs->getINodeID(path);
    if (inode_id == 0) return -1;

    // Read INode properties
    INode inode = fs->getINode(inode_id);
    info->st_atime   = inode.atime;
    info->st_ctime   = inode.ctime;
    info->st_mtime   = inode.mtime;
    info->st_size    = inode.size;
    info->st_blocks  = inode.blocks;
    info->st_nlink   = inode.links;
    info->st_gid     = inode.gid;
    info->st_uid     = inode.uid;
    info->st_mode    = inode.mode;
    info->st_ino     = inode_id;
    info->st_blksize = Block::SIZE;
    info->st_dev     = inode.dev;
    // info->st_rdev = inode.rdev;

    // Modify mode depending on file type
    if (inode.type == FileType::REGULAR) {
      info->st_mode = info->st_mode | S_IFDIR;
    } else if (inode.type == FileType::DIRECTORY) {
      info->st_mode = info->st_mode | S_IFREG;
    } else if (inode.type == FileType::SYMLINK) {
      info->st_mode = info->st_mode | S_IFLNK;
    } else {
      throw std::out_of_range("Called getattr on a free INode!");
    }
    return 0;
  }

  int fs_getxattr(const char* path, const char* attr, char* buffer, size_t size) {
    debug("getxattr    %s %s\n", path, attr);
    UNUSED(buffer);
    UNUSED(size);

    // Not implemented!
    return -1;
  }

  void* fs_init(struct fuse_conn_info* conn) {
    UNUSED(conn);

    // Useless function for us
    return NULL;
  }

  int fs_link(const char* target, const char* link) {
    debug("link        %s -> %s\n", link, target);

    INode::ID inode_id = fs->getINodeID(target);
    if (inode_id == 0) return -1;

    std::string dname = fs->dirname(link);
    std::string fname = fs->basename(link);

    // Get the link's directory
    Directory dir = fs->getDirectory(dname);
    if(dir.contains(fname)) return -1;

    // Update the target INode's link count
    INode inode = fs->getINode(inode_id);
    inode.ctime = time(NULL);
    inode.links += 1;
    fs->save(inode_id, inode);

    dir.insert(fname, inode_id);
    fs->save(dir);
    return 0;
  }

  int fs_listxattr(const char* path, char* buffer, size_t size) {
    debug("listxattr   %s\n", path);
    UNUSED(buffer);
    UNUSED(size);

    // Not implemented!
    return -1;
  }

  int fs_mkdir(const char* path, mode_t mode) {
    debug("mkdir       %s %03o\n", path, mode);

    // Check if path exists - if so, don't overwrite it
    if (fs->getINodeID(path) != 0) return -1;

    // Check if path's parent directory exists
    std::string parent_dname = fs->dirname(path);
    std::string dname = fs->basename(path);
    INode::ID parent_dir_id = fs->getINodeID(parent_dname);
    if (parent_dir_id == 0) return -1;

    // Allocate an inode for new directory and write in parent directory
    INode::ID new_dir_inode_id = inode_manager->reserve();
    Directory dir = fs->getDirectory(parent_dir_id);
    dir.insert(dname, new_dir_inode_id);
    fs->save(dir);

    // Set the new directory's attributes
    INode new_dir_inode(FileType::DIRECTORY, mode);
    fs->save(new_dir_inode_id, new_dir_inode);

    // Initialize the new directory's contents
    Directory new_dir(new_dir_inode_id, parent_dir_id);
    fs->save(new_dir);

    // TODO: How do we set these?
    // new_dir_inode.uid = ???
    // new_dir_inode.gid = ???
    // new_dir_inode.flags = ???

    return 0;
  }

  int fs_mknod(const char* path, mode_t mode, dev_t dev) {
    debug("mknod       %s %03o\n", path, mode);

    // Check if path exists - if so, don't overwrite it
    if (fs->getINodeID(path) != 0) return -1;

    std::string dname = fs->dirname(path);
    std::string fname = fs->basename(path);

    // Check if path's parent directory exists
    INode::ID parent_inode_id = fs->getINodeID(dname);
    if (parent_inode_id == 0) return -1;

    // Allocate an inode for new file and write in parent directory
    INode::ID new_file_inode_id = inode_manager->reserve();
    Directory dir = fs->getDirectory(parent_inode_id);
    dir.insert(fname, new_file_inode_id);
    fs->save(dir);

    // Set the new file's attributes
    INode new_file_inode(FileType::REGULAR, mode, dev);
    fs->save(new_file_inode_id, new_file_inode);
    return 0;
  }

  int fs_open(const char* path, fuse_file_info* info) {
    debug("open        %s\n", path);
    UNUSED(info);

    // TODO...
    return 0;
  }

  int fs_read(const char* path, char* buffer, size_t size, off_t offset, fuse_file_info* info) {
    debug("read        %s %" PRIu64 "b at %" PRId64 "\n", path, (uint64_t) size, offset);
    UNUSED(info);

    // Check if file exists
    INode::ID id = fs->getINodeID(path);
    if(id == 0) return -1;

    // Make sure it's a regular file
    INode inode = fs->getINode(id);
    if(inode.type != FileType::REGULAR) {
      return -1;
    }

    // Read data
    return fs->read(id, buffer, size, offset);
  }

  int fs_readdir(const char* path, void* buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* info) {
    debug("readdir     %s\n", path);
    UNUSED(offset);
    UNUSED(info);

    Directory dir = fs->getDirectory(path);
    for(const auto itr: dir.entries()) {
      int result = filler(buffer, itr.first.c_str(), NULL, 0);
      if(result != 0) return result;
    }

    return 0;
  }

  int fs_readlink(const char* path, char* buffer, size_t size) {
    debug("readlink    %s\n", path);

    // Check if file exists
    INode::ID id = fs->getINodeID(path);
    if(id == 0) return -1;

    // Make sure it's a symlink
    INode inode = fs->getINode(id);
    if(inode.type != FileType::SYMLINK) {
      return -1;
    }

    return fs->read(id, buffer, size, 0);
  }

  int fs_release(const char* path, fuse_file_info* info) {
    debug("release     %s\n", path);
    UNUSED(info);

    // TODO...
    return 0;
  }

  int fs_removexattr(const char* path, const char* attr) {
    debug("removexattr %s %s\n", path, attr);

    // Not implemented!
    return -1;
  }

  int fs_rename(const char* path, const char* name) {
    debug("rename      %s -> %s\n", path, name);

    int result = fs_link(path, name);
    if (result < 0) return result;
    return fs_unlink(path);
  }

  int fs_rmdir(const char* path) {
    debug("rmdir       %s\n", path);

    std::string pname = fs->dirname(path);
    std::string dname = fs->basename(path);

    Directory parent = fs->getDirectory(pname);
    INode::ID id     = parent.search(dname);
    Directory dir    = fs->getDirectory(id);
    if(!dir.isEmpty()) return -1;

    parent.remove(dname);
    fs->save(parent);

    fs->unlink(id);
    return 0;
  }

  int fs_setxattr(const char* path, const char* attr, const char* val, size_t size, int unknown) {
    debug("setxattr    %s %s\n", path, attr);
    UNUSED(val);
    UNUSED(size);
    UNUSED(unknown);

    // Not implemented!
    return -1;
  }

  int fs_statfs(const char* path, struct statvfs* info) {
    debug("statfs      %s\n", path);

    fs->statfs(info);
    return 0;
  }

  int fs_symlink(const char* target, const char* link) {
    debug("symlink     %s -> %s\n", link, target);

    std::string dname = fs->dirname(link);
    std::string fname = fs->basename(link);

    Directory dir = fs->getDirectory(dname);
    if(dir.contains(fname)) return -1;

    INode::ID id = inode_manager->reserve();
    INode inode(FileType::SYMLINK, 0777);
    fs->write(id, target, std::strlen(target) + 1, 0);
    fs->save(id, inode);

    dir.insert(fname, id);
    fs->save(dir);
    return 0;
  }

  int fs_truncate(const char* path, off_t offset) {
    debug("truncate    %s to %" PRId64 "b\n", path, (int64_t) offset);

    // Check if file exists
    INode::ID id = fs->getINodeID(path);
    if(id == 0) return -1;

    // Make sure it's a regular file
    INode inode = fs->getINode(id);
    if(inode.type != FileType::REGULAR) {
      return -1;
    }

    // Cut data
    return fs->truncate(id, offset);
  }

  int fs_unlink(const char* path) {
    debug("unlink      %s\n", path);

    std::string dname = fs->dirname(path);
    std::string fname = fs->basename(path);

    Directory dir = fs->getDirectory(dname);
    INode::ID fid = dir.search(fname);
    if(fid == 0) return -1;

    dir.remove(fname);
    fs->save(dir);

    fs->unlink(fid);
    return 0;
  }

  // int(* fuse_operations::utimens) (const char *, const struct timespec tv[2], struct fuse_file_info *fi)
  // This supersedes the old utime() interface. New applications should use this.
  int fs_utime(const char* path, utimbuf* buffer) {
    debug("utime       %s\n", path);

    // Check if path exists
    INode::ID inode_id = fs->getINodeID(path);
    if (inode_id == 0) return -1;

    // Update INode
    INode inode = fs->getINode(inode_id);
    if (buffer->actime  == 0) buffer->actime  = time(NULL);
    if (buffer->modtime == 0) buffer->modtime = time(NULL);
    inode.atime = buffer->actime;
    inode.mtime = buffer->modtime;
    inode.ctime = time(NULL);
    fs->save(inode_id, inode);
    return 0;
  }

  int fs_write(const char* path, const char* data, size_t size, off_t offset, fuse_file_info* info) {
    debug("write       %s %" PRIu64 "b at %" PRId64 "\n", path, (uint64_t) size, offset);
    UNUSED(info);

    // Check if file exists
    INode::ID id = fs->getINodeID(path);
    if(id == 0) return -1;

    // Make sure it's a regular file
    INode inode = fs->getINode(id);
    if(inode.type != FileType::REGULAR) {
      return -1;
    }

    // Write data
    return fs->write(id, data, size, offset);
  }
}

int main(int argc, char** argv) {
  // added initialization for fuse arguments
  // can also be changed to 0, NULL for testing empty
  // struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

  // Default ~32GB disk
  // TODO: Read value from argv
  uint64_t nblocks = 1 + 10 + (1 + 512) + (1 + 512 + 512*512) + (1 + 2 + 512*2 + 512*512*2);

  // Instantiate objects for filesystem
  Storage *disk = new FileStorage("/dev/vdc", nblocks);
  BlockManager *block_manager = new StackBasedBlockManager(*disk);
  inode_manager = new LinearINodeManager(*disk);
  fs = new Filesystem(*block_manager, *inode_manager, *disk);

  // Set FUSE function pointers
  fuse_operations ops;
  memset(&ops, 0, sizeof(ops));

  ops.chmod       = &fs_chmod;
  ops.chown       = &fs_chown;
  // ops.destroy     = &fs_destroy;
  // ops.flush       = &fs_flush;
  // ops.fsync       = &fs_fsync;
  // ops.fsyncdir    = &fs_fsyncdir;
  ops.getattr     = &fs_getattr;
  // ops.getxattr    = &fs_getxattr;
  ops.init        = &fs_init;
  ops.link        = &fs_link;
  // ops.listxattr   = &fs_listxattr;
  ops.mkdir       = &fs_mkdir;
  ops.mknod       = &fs_mknod;
  ops.open        = &fs_open;
  // ops.opendir     = &fs_opendir;
  ops.read        = &fs_read;
  ops.readdir     = &fs_readdir;
  ops.readlink    = &fs_readlink;
  ops.release     = &fs_release;
  // ops.releasedir  = &fs_releasedir;
  // ops.removexattr = &fs_removexattr;
  ops.rename      = &fs_rename;
  ops.rmdir       = &fs_rmdir;
  // ops.setxattr    = &fs_setxattr;
  ops.statfs      = &fs_statfs;
  ops.symlink     = &fs_symlink;
  ops.truncate    = &fs_truncate;
  ops.unlink      = &fs_unlink;
  ops.utime       = &fs_utime;
  ops.write       = &fs_write;

  // Run the FUSE daemon!
  return fuse_main(argc, argv, &ops, NULL);
}

/*
  References:
  1. relatime
    - http://wisercoder.com/knowing-difference-mtime-ctime-atime/
    - https://superuser.com/questions/464290/why-is-cat-not-changing-the-access-time
*/
