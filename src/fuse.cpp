#include "lib/Filesystem.h"

#include <fuse.h>
struct statfs;
struct stat;

#ifndef NDEBUG
  #include <cstdio>
  #define debug(format, ...) fprintf(stderr, format, __VA_ARGS__)
#else
  #define debug(...)
#endif


extern "C" {
  static Filesystem* FILESYSTEM = 0;

  int fs_chmod(const char* path, mode_t mode) {
    debug("chmod       %s to %03o\n", path, mode);
    return FILESYSTEM->chmod(path, mode);
  }

  int fs_chown(const char* path, uid_t uid, gid_t gid) {
    debug("chown       %s to %d:%d\n", path, uid, gid);
    return FILESYSTEM->chown(path, uid, gid);
  }

  int fs_flush(const char* path, fuse_file_info* info) {
    debug("flush       %s\n", path);
    return FILESYSTEM->flush(path, info);
  }

  int fs_fsync(const char* path, int unknown, fuse_file_info* info) {
    debug("fsync       %s\n", path);
    return FILESYSTEM->fsync(path, unknown, info);
  }

  int fs_getattr(const char* path, stat* info) {
    debug("getattr     %s\n", path);
    return FILESYSTEM->getattr(path, info);
  }

  int fs_getdir(const char* path, fuse_dirh_t dirh, fuse_dirfil_t dirfil) {
    debug("getdir      %s\n", path);
    return FILESYSTEM->getdir(path, dirh, dirfil);
  }

  int fs_getxattr(const char* path, const char* attr, char* buffer, size_t size) {
    debug("getxattr    %s %s\n", path, attr);
    // return FILESYSTEM->getxattr(path, attr, buffer, size);
    return -1;
  }

  int fs_link(const char* path, const char* link) {
    debug("link        %s -> %s\n", path, link);
    return FILESYSTEM->link(path, link);
  }

  int fs_listxattr(const char* path, char* buffer, size_t size) {
    debug("listxattr   %s\n", path);
    // return FILESYSTEM->listxattr(path, buffer, size);
    return -1;
  }

  int fs_mkdir(const char* path, mode_t mode) {
    debug("mkdir       %s %03o\n", path, mode);
    return FILESYSTEM->mkdir(path, mode);
  }

  int fs_mknod(const char* path, mode_t mode, dev_t dev) {
    debug("mknod       %s %03o\n", path, mode);
    return FILESYSTEM->mknod(path, mode, dev);
  }

  int fs_open(const char* path, fuse_file_info* info) {
    debug("open        %s\n", path);
    return FILESYSTEM->open(path, info);
  }

  int fs_read(const char* path, char* buffer, size_t size, off_t offset, fuse_file_info* info) {
    debug("read        %s %zdb at %zd\n", path, (int64_t) size, (int64_t) offset);
    return FILESYSTEM->read(path, buffer, size, offset, info);
  }

  int fs_readlink(const char* path, char* buffer, size_t size) {
    debug("readlink    %s\n", path);
    return FILESYSTEM->readlink(path, buffer, size);
  }

  int fs_release(const char* path, fuse_file_info* info) {
    debug("release     %s\n", path);
    return FILESYSTEM->release(path, info);
  }

  int fs_removexattr(const char* path, const char* attr) {
    debug("removexattr %s %s\n", path, attr);
    // return FILESYSTEM->removexattr(path, attr);
    return -1;
  }

  int fs_rename(const char* path, const char* name) {
    debug("rename    %s -> %s\n", path, name);
    return FILESYSTEM->rename(path, name);
  }

  int fs_rmdir(const char* path) {
    debug("rmdir     %s\n", path);
    return FILESYSTEM->rmdir(path);
  }

  int fs_setxattr(const char* path, const char* attr, const char* val, size_t size, int unknown) {
    debug("setxattr  %s %s\n", path, attr);
    // return FILESYSTEM->setxattr(path, attr, val, size, unknown);
    return -1;
  }

  int fs_statfs(const char* path, statfs* info) {
    debug("statfs    %s\n", path);
    return FILESYSTEM->statfs(path, info);
  }

  int fs_symlink(const char* path, const char* link) {
    debug("symlink   %s -> %s\n", path, link);
    return FILESYSTEM->symlink(path, link);
  }

  int fs_truncate(const char* path, off_t offset) {
    debug("truncate  %s to %zdb\n", path, (int64_t) offset);
    return FILESYSTEM->truncate(path, offset);
  }

  int fs_unlink(const char* path) {
    debug("unlink    %s\n", path);
    return FILESYSTEM->unlink(path);
  }

  int fs_utime(const char* path, utimbuf* buffer) {
    debug("utime     %s\n", path);
    return FILESYSTEM->utime(path, buffer);
  }

  int fs_write(const char* path, const char* data, size_t size, off_t offset, fuse_file_info* info) {
    debug("write     %s %zdb at %zd\n", path, (int64_t) size, (int64_t) offset);
    return FILESYSTEM->write(path, data, size, offset, info);
  }

  int main(int argc, char** argv) {
    const fuse_operations ops {
      .getattr     = &fs_getattr,
      .readlink    = &fs_readlink,
      .mknod       = &fs_mknod,
      .mkdir       = &fs_mkdir,
      .unlink      = &fs_unlink,
      .rmdir       = &fs_rmdir,
      .symlink     = &fs_symlink,
      .rename      = &fs_rename,
      .link        = &fs_link,
      .chmod       = &fs_chmod,
      .chown       = &fs_chown,
      .truncate    = &fs_truncate,
      .open        = &fs_open,
      .read        = &fs_read,
      .write       = &fs_write,
      .statfs      = &fs_statfs,
      .flush       = &fs_flush,
      .release     = &fs_release,
      .fsync       = &fs_fsync,
      .setxattr    = &fs_setxattr,
      .getxattr    = &fs_getxattr,
      .listxattr   = &fs_listxattr,
      .removexattr = &fs_removexattr

      // .opendir     = &FUSE::opendir,
      // .readdir     = &FUSE::readdir,
      // .releasedir  = &FUSE::releasedir,
      // .fsyncdir    = &FUSE::fsyncdir,

      // .init        = &FUSE::init,
      // .destroy     = &FUSE::destroy,

      // .getdir      = &FUSE::getdir,
      // .utime       = &FUSE::utime,
    };

    FUSE::FILESYSTEM = new Filesystem;
    return fuse_main(argc, argv, &ops);
  }
}
