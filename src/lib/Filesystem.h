#ifndef SIGSEGV_FILESYSTEM_H
#define SIGSEGV_FILESYSTEM_H

#include "BlockManager.h"
#include "INodeManager.h"

//#include <sys/statfs.h>
#include <sys/stat.h>
#include <fuse.h>

class Filesystem {
  BlockManager& blocks;
  INodeManager& inodes;

public:
  Filesystem(BlockManager& b, INodeManager& i): blocks(b), inodes(i) {
    // All done.
  }

  // FUSE Operations:
  int chmod(const char*, mode_t);
  int chown(const char*, uid_t, gid_t);
  int flush(const char*, fuse_file_info*);
  int fsync(const char*, int, fuse_file_info*);
  int getattr(const char*, struct stat*);
  int getdir(const char*, fuse_dirh_t, fuse_dirfil_t);
  int getxattr(const char*, const char*, char*, size_t);
  int link(const char*, const char*);
  int listxattr(const char*, char*, size_t);
  int mkdir(const char*, mode_t);
  int mknod(const char*, mode_t, dev_t);
  int open(const char*, fuse_file_info*);
  int read(const char*, char*, size_t, off_t, fuse_file_info*);
  int readlink(const char*, char*, size_t);
  int release(const char*, fuse_file_info*);
  int removexattr(const char*, const char*);
  int rename(const char*, const char*);
  int rmdir(const char*);
  int setxattr(const char*, const char*, const char*, size_t, int);
  int statfs(const char*, struct statfs*);
  int symlink(const char*, const char*);
  int truncate(const char*, off_t);
  int unlink(const char*);
  int utime(const char*, utimbuf*);
  int write(const char*, const char*, size_t, off_t, fuse_file_info*);
};

#endif
