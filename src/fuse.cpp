#include "lib/Filesystem.h"

#include <sys/statfs.h>
#include <sys/vfs.h>
#include <fuse.h>
#include <string.h>

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

  int fs_getattr(const char* path, struct stat* info) {
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

  int fs_statfs(const char* path, struct statvfs* info) {
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
}

int main(int argc, char** argv) {
  fuse_operations ops;
  memset(&ops, 0, sizeof(ops));

  ops.chmod       = &fs_chmod;
  ops.chown       = &fs_chown;
  ops.flush       = &fs_flush;
  ops.fsync       = &fs_fsync;
  ops.getattr     = &fs_getattr;
  ops.getxattr    = &fs_getxattr;
  ops.link        = &fs_link;
  ops.listxattr   = &fs_listxattr;
  ops.mkdir       = &fs_mkdir;
  ops.mknod       = &fs_mknod;
  ops.open        = &fs_open;
  ops.read        = &fs_read;
  ops.readlink    = &fs_readlink;
  ops.release     = &fs_release;
  ops.removexattr = &fs_removexattr;
  ops.rename      = &fs_rename;
  ops.rmdir       = &fs_rmdir;
  ops.setxattr    = &fs_setxattr;
  ops.statfs      = &fs_statfs;
  ops.symlink     = &fs_symlink;
  ops.truncate    = &fs_truncate;
  ops.unlink      = &fs_unlink;
  ops.write       = &fs_write;

  // .opendir     = &FUSE::opendir,
  // .readdir     = &FUSE::readdir,
  // .releasedir  = &FUSE::releasedir,
  // .fsyncdir    = &FUSE::fsyncdir,

  // .init        = &FUSE::init,
  // .destroy     = &FUSE::destroy,

  // .getdir      = &FUSE::getdir,
  // .utime       = &FUSE::utime,

  FILESYSTEM = new Filesystem;
  return fuse_main(argc, argv, &ops, NULL);
}
