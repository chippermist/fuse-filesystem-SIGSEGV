extern "C" {
  int chmod(const char* path, mode_t mode) {
    FILESYSTEM->chmod(path, mode);
  }

  int chown(const char* path, uid_t uid, gid_t gid) {
    FILESYSTEM->chown(path, uid, gid);
  }

  int flush(const char* path, fuse_file_info* info) {
    FILESYSTEM->flush(path, info);
  }

  int fsync(const char* path, int, fuse_file_info* info) {
    FILESYSTEM->fsync(path, int, info);
  }

  int getattr(const char* path, stat* info) {
    FILESYSTEM->getattr(path, info);
  }

  int getdir(const char* path, fuse_dirh_t dirh, fuse_dirfil_t dirfil) {
    FILESYSTEM->getdir(path, dirh, dirfil);
  }

  // int getxattr(const char* path, const char*, char*, size_t) {
  //   FILESYSTEM->getxattr(path, const char*, char*, size_t)
  // }

  int link(const char* path, const char* link) {
    FILESYSTEM->link(path, link);
  }

  // int listxattr(const char* path, char* buffer, size_t size) {
  //   FILESYSTEM->listxattr(path, buffer, size)
  // }

  int mkdir(const char* path, mode_t mode) {
    FILESYSTEM->mkdir(path, mode);
  }

  int mknod(const char* path, mode_t mode, dev_t dev) {
    FILESYSTEM->mknod(path, mode, dev);
  }

  int open(const char* path, fuse_file_info* info) {
    FILESYSTEM->open(path, info);
  }

  int read(const char* path, char* buffer, size_t size, off_t offset, fuse_file_info* info) {
    FILESYSTEM->read(path, buffer, size, offset, info);
  }

  int readlink(const char* path, char* buffer, size_t size) {
    FILESYSTEM->readlink(path, buffer, size);
  }

  int release(const char* path, fuse_file_info* info) {
    FILESYSTEM->release(path, info);
  }

  // int removexattr(const char* path, const char*) {
  //   FILESYSTEM->removexattr(path, const char*)
  // }

  int rename(const char* path, const char* name) {
    FILESYSTEM->rename(path, name);
  }

  int rmdir(const char* path) {
    FILESYSTEM->rmdir(path);
  }

  // int setxattr(const char* path, path const char*, const char*, size_t, int) {
  //   FILESYSTEM->setxattr(path, const char*, const char*, size_t, int)
  // }

  int statfs(const char* path, statfs* info) {
    FILESYSTEM->statfs(path, info);
  }

  int symlink(const char* path, const char* link) {
    FILESYSTEM->symlink(path, link);
  }

  int truncate(const char* path, off_t offset) {
    FILESYSTEM->truncate(path, offset);
  }

  int unlink(const char* path) {
    FILESYSTEM->unlink(path);
  }

  int utime(const char* path, utimbuf* buffer) {
    FILESYSTEM->utime(path, buffer);
  }

  int write(const char* path, const char* data, size_t size, off_t offset, fuse_file_info* info) {
    FILESYSTEM->write(path, data, size, offset, info);
  }
}
