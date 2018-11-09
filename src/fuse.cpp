#include "lib/Filesystem.h"

#include <sys/statfs.h>
#include <sys/stat.h>
#include <fuse.h>

#ifndef NDEBUG
  #include <cstdio>
  #define debug(format, ...) fprintf(stderr, format, __VA_ARGS__)
#else
  #define debug(...)
#endif

namespace FUSE {
  static Filesystem* FILESYSTEM = 0;

  int chmod(const char* path, mode_t mode) {
    debug("chmod       %s to %03o\n", path, mode);
    return FILESYSTEM->chmod(path, mode);
  }

  int chown(const char* path, uid_t uid, gid_t gid) {
    debug("chown       %s to %d:%d\n", path, uid, gid);
    return FILESYSTEM->chown(path, uid, gid);
  }

  int flush(const char* path, fuse_file_info* info) {
    debug("flush       %s\n", path);
    return FILESYSTEM->flush(path, info);
  }

  int fsync(const char* path, int unknown, fuse_file_info* info) {
    debug("fsync       %s\n", path);
    return FILESYSTEM->fsync(path, unknown, info);
  }

  int getattr(const char* path, stat* info) {
    debug("getattr     %s\n", path);
    return FILESYSTEM->getattr(path, info);
  }

  int getdir(const char* path, fuse_dirh_t dirh, fuse_dirfil_t dirfil) {
    debug("getdir      %s\n", path);
    return FILESYSTEM->getdir(path, dirh, dirfil);
  }

  int getxattr(const char* path, const char* attr, char* buffer, size_t size) {
    debug("getxattr    %s %s\n", path, attr);
    // return FILESYSTEM->getxattr(path, attr, buffer, size);
    return -1;
  }

  int link(const char* path, const char* link) {
    debug("link        %s -> %s\n", path, link);
    return FILESYSTEM->link(path, link);
  }

  int listxattr(const char* path, char* buffer, size_t size) {
    debug("listxattr   %s\n", path);
    // return FILESYSTEM->listxattr(path, buffer, size);
    return -1;
  }

  int mkdir(const char* path, mode_t mode) {
    debug("mkdir       %s %03o\n", path, mode);
    return FILESYSTEM->mkdir(path, mode);
  }

  int mknod(const char* path, mode_t mode, dev_t dev) {
    debug("mknod       %s %03o\n", path, mode);
    return FILESYSTEM->mknod(path, mode, dev);
  }

  int open(const char* path, fuse_file_info* info) {
    debug("open        %s\n", path);
    return FILESYSTEM->open(path, info);
  }

  int read(const char* path, char* buffer, size_t size, off_t offset, fuse_file_info* info) {
    debug("read        %s %lldb at %lld\n", path, (int64_t) size, (int64_t) offset);
    return FILESYSTEM->read(path, buffer, size, offset, info);
  }

  int readlink(const char* path, char* buffer, size_t size) {
    debug("readlink    %s\n", path);
    return FILESYSTEM->readlink(path, buffer, size);
  }

  int release(const char* path, fuse_file_info* info) {
    debug("release     %s\n", path);
    return FILESYSTEM->release(path, info);
  }

  int removexattr(const char* path, const char* attr) {
    debug("removexattr %s %s\n", path, attr);
    // return FILESYSTEM->removexattr(path, attr);
    return -1;
  }

  int rename(const char* path, const char* name) {
    debug("rename    %s -> %s\n", path, name);
    return FILESYSTEM->rename(path, name);
  }

  int rmdir(const char* path) {
    debug("rmdir     %s\n", path);
    return FILESYSTEM->rmdir(path);
  }

  int setxattr(const char* path, const char* attr, const char* val, size_t size, int unknown) {
    debug("setxattr  %s %s\n", path, attr);
    // return FILESYSTEM->setxattr(path, attr, val, size, unknown);
    return -1;
  }

  int statfs(const char* path, statfs* info) {
    debug("statfs    %s\n", path);
    return FILESYSTEM->statfs(path, info);
  }

  int symlink(const char* path, const char* link) {
    debug("symlink   %s -> %s\n", path, link);
    return FILESYSTEM->symlink(path, link);
  }

  int truncate(const char* path, off_t offset) {
    debug("truncate  %s to %lldb\n", path, (int64_t) offset);
    return FILESYSTEM->truncate(path, offset);
  }

  int unlink(const char* path) {
    debug("unlink    %s\n", path);
    return FILESYSTEM->unlink(path);
  }

  int utime(const char* path, utimbuf* buffer) {
    debug("utime     %s\n", path);
    return FILESYSTEM->utime(path, buffer);
  }

  int write(const char* path, const char* data, size_t size, off_t offset, fuse_file_info* info) {
    debug("write     %s %lldb at %lld\n", path, (int64_t) size, (int64_t) offset);
    return FILESYSTEM->write(path, data, size, offset, info);
  }
}

int main(int argc, char** argv) {
  const fuse_operations ops {
    .getattr     = &FUSE::getattr,
    .readlink    = &FUSE::readlink,
    .mknod       = &FUSE::mknod,
    .mkdir       = &FUSE::mkdir,
    .unlink      = &FUSE::unlink,
    .rmdir       = &FUSE::rmdir,
    .symlink     = &FUSE::symlink,
    .rename      = &FUSE::rename,
    .link        = &FUSE::link,
    .chmod       = &FUSE::chmod,
    .chown       = &FUSE::chown,
    .truncate    = &FUSE::truncate,
    .open        = &FUSE::open,
    .read        = &FUSE::read,
    .write       = &FUSE::write,
    .statfs      = &FUSE::statfs,
    .flush       = &FUSE::flush,
    .release     = &FUSE::release,
    .fsync       = &FUSE::fsync,
    .setxattr    = &FUSE::setxattr,
    .getxattr    = &FUSE::getxattr,
    .listxattr   = &FUSE::listxattr,
    .removexattr = &FUSE::removexattr

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
