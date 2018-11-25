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
  // int(* fuse_operations::chmod) (const char *, mode_t, struct fuse_file_info *fi)
  int fs_chmod(const char* path, mode_t mode) {
    debug("chmod       %s to %03o\n", path, mode);

    INode inode = FileAccessManager::getINodeFromPath(path);
    inode.data.mode = mode;
    inode.save();
    return 0;
  }

  // int(* fuse_operations::chown) (const char *, uid_t, gid_t, struct fuse_file_info *fi)
  int fs_chown(const char* path, uid_t uid, gid_t gid) {
    debug("chown       %s to %d:%d\n", path, uid, gid);

    INode inode = FileAccessManager::getINodeFromPath(path);
    inode.data.uid = uid;
    inode.data.gid = gid;
    inode.save();
    return 0;
  }

  // int(* fuse_operations::flush) (const char *, struct fuse_file_info *)
  int fs_flush(const char* path, fuse_file_info* info) {
    debug("flush       %s\n", path);

    // TODO...
    return 0;
  }

  // int(* fuse_operations::fsync) (const char *, int, struct fuse_file_info *)
  int fs_fsync(const char* path, int unknown, fuse_file_info* info) {
    debug("fsync       %s\n", path);

    // TODO...
    return 0;
  }

  // int(* fuse_operations::getattr) (const char *, struct stat *, struct fuse_file_info *fi)
  int fs_getattr(const char* path, struct stat* info) {
    debug("getattr     %s\n", path);

    INode inode = FileAccessManager::getINodeFromPath(path);
    // TODO...
    return 0;
  }

  // did not find a definition in fuse::operations
  int fs_getdir(const char* path, fuse_dirh_t dirh, fuse_dirfil_t dirfil) {
    debug("getdir      %s\n", path);

    Directory dir = Directory::get(path);
    // TODO...
    return 0;
  }

  // int(* fuse_operations::getxattr) (const char *, const char *, char *, size_t)
  int fs_getxattr(const char* path, const char* attr, char* buffer, size_t size) {
    debug("getxattr    %s %s\n", path, attr);
    // Not implemented!
    return -1;
  }

  // int(* fuse_operations::link) (const char *, const char *)
  int fs_link(const char* path, const char* link) {
    debug("link        %s -> %s\n", path, link);

    std::string dname = dirname(path);
    std::string fname = basename(path);

    INode::ID id = INode::id(link);
    if(id == 0) return -ENOENT;

    Directory dir = Directory::get(dname);
    dir[fname] = id;
    dir.save();
    return 0;
  }

  // int(* fuse_operations::listxattr) (const char *, char *, size_t)
  int fs_listxattr(const char* path, char* buffer, size_t size) {
    debug("listxattr   %s\n", path);
    // Not implemented!
    return -1;
  }

  // int(* fuse_operations::mkdir) (const char *, mode_t)
  int fs_mkdir(const char* path, mode_t mode) {
    debug("mkdir       %s %03o\n", path, mode);

    // TODO...
    return 0;
  }

  // int(* fuse_operations::mknod) (const char *, mode_t, dev_t)
  int fs_mknod(const char* path, mode_t mode, dev_t dev) {
    debug("mknod       %s %03o\n", path, mode);

    // TODO...
    return 0;
  }

  // int(* fuse_operations::open) (const char *, struct fuse_file_info *)
  int fs_open(const char* path, fuse_file_info* info) {
    debug("open        %s\n", path);

    // TODO...
    return 0;
  }

  // int(* fuse_operations::open) (const char *, struct fuse_file_info *)
  int fs_read(const char* path, char* buffer, size_t size, off_t offset, fuse_file_info* info) {
    debug("read        %s %zdb at %zd\n", path, (int64_t) size, (int64_t) offset);

    INode inode = FileAccessManager::getINodeFromPath(path);
    inode.read(buffer, size, offset);
    return 0;
  }

  // int(* fuse_operations::readlink) (const char *, char *, size_t)
  int fs_readlink(const char* path, char* buffer, size_t size) {
    debug("readlink    %s\n", path);

    // Could possible use readlink() from <unistd.h>

    // int res = readlink(path, buffer, size-1);
    // if(res == -1) {
    //   return -1;
    // }
    // buffer[res] = '\0'  // Null terminator
    // return 0;  //on success

    INode inode = INode::get(path);
    // TODO: What's the correct error code here?
    if(inode.size > size) {
      return -1;
    }
    inode.read(buffer, size, 0);
    // TODO: Should this return the number of bytes read?
    return 0;
  }

  // int(* fuse_operations::release) (const char *, struct fuse_file_info *)
  int fs_release(const char* path, fuse_file_info* info) {
    debug("release     %s\n", path);

    // TODO...
    return 0;
  }

  // int(* fuse_operations::removexattr) (const char *, const char *)
  int fs_removexattr(const char* path, const char* attr) {
    debug("removexattr %s %s\n", path, attr);
    // Not implemented!
    return -1;
  }

  // int(* fuse_operations::rename) (const char *, const char *, unsigned int flags)
  int fs_rename(const char* path, const char* name) {
    debug("rename      %s -> %s\n", path, name);

    int result = fs_link(path, name);
    if(result != 0) return result;
    return fs_unlink(path);
  }

  // int(* fuse_operations::rmdir) (const char *)
  int fs_rmdir(const char* path) {
    debug("rmdir       %s\n", path);

    std::string pname = dirname(path);
    std::string dname = basename(path);

    Directory parent = Directory::get(pname);
    parent.remove(dname);
    parent.save();
    return 0;
  }

  // int(* fuse_operations::setxattr) (const char *, const char *, const char *, size_t, int)
  int fs_setxattr(const char* path, const char* attr, const char* val, size_t size, int unknown) {
    debug("setxattr    %s %s\n", path, attr);
    // Not implemented!
    return -1;
  }

  // int(* fuse_operations::statfs) (const char *, struct statvfs *)
  int fs_statfs(const char* path, struct statvfs* info) {
    debug("statfs      %s\n", path);

    int status = statvfs(path, info);
    if(status == -1) {
      return -1;
    }
    // TODO...
    return 0;
  }

  // int(* fuse_operations::symlink) (const char *, const char *)
  int fs_symlink(const char* path, const char* link) {
    debug("symlink     %s -> %s\n", path, link);

    std::string dname = dirname(path);
    std::string fname = basename(path);

    INode inode = INode::reserve();
    inode.data.type = SYMLINK;
    inode.write(link, strlen(link), 0);

    Directory dir = Directory::get(dname);
    dir[fname] = inode;
    dir.save();
    return 0;
  }

  // int(* fuse_operations::truncate) (const char *, off_t, struct fuse_file_info *fi)
  int fs_truncate(const char* path, off_t offset) {
    debug("truncate    %s to %zdb\n", path, (int64_t) offset);

    // TODO...
    return 0;
  }

  // int(* fuse_operations::unlink) (const char *)
  int fs_unlink(const char* path) {
    debug("unlink      %s\n", path);

    std::string dname = dirname(path);
    std::string fname = basename(path);

    Directory dir = Directory::get(dname);
    dir.remove(fname);
    dir.save();
    return 0;
  }

  // int(* fuse_operations::utimens) (const char *, const struct timespec tv[2], struct fuse_file_info *fi)
  // This supersedes the old utime() interface. New applications should use this.
  int fs_utime(const char* path, utimbuf* buffer) {
    debug("utime       %s\n", path);

    // Get the inode using the path
    INode inode = FileAccessManager::getINodeFromPath(path);

    // Update the times
    inode.time  = buffer[0];  // update access time
    inode.mtime = buffer[1];  // update modification time
    
    // Set the changes back to inode
    inode.save();
    return 0;
  }

  // int(* fuse_operations::write) (const char *, const char *, size_t, off_t, struct fuse_file_info *)
  int fs_write(const char* path, const char* data, size_t size, off_t offset, fuse_file_info* info) {
    debug("write       %s %zdb at %zd\n", path, (int64_t) size, (int64_t) offset);

    INode inode = FileAccessManager::getINodeFromPath(path);
    inode.write(data, size, offset);
    // TODO: Should this return the number of bytes written?
    return 0;
  }


  // void*(* fuse_operations::init) (struct fuse_conn_info *conn, struct fuse_config *cfg)
  int fs_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
    (void) conn;
    cfg->kernel_cache = 1;
    return NULL;
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

  // ops.opendir     = &fs_opendir;
  // ops.readdir     = &fs_readdir;
  // ops.releasedir  = &fs_releasedir;
  // ops.fsyncdir    = &fs_fsyncdir;

  ops.init        = &fs_init;
  // ops.destroy     = &fs_destroy;

  // ops.getdir      = &fs_getdir;
  ops.utime       = &fs_utime;

  return fuse_main(argc, argv, &ops, NULL);
}