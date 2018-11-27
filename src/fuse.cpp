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

// Global Variables for objects
Storage *disk;
BlockManager *block_manager;
INodeManager *inode_manager;
FileAccessManager *file_access_manager;

extern "C" {
  // int(* fuse_operations::chmod) (const char *, mode_t, struct fuse_file_info *fi)
  int fs_chmod(const char* path, mode_t mode) {
    debug("chmod       %s to %03o\n", path, mode);

    // Check if path exists
    INode::ID inode_id = file_access_manager->getINodeFromPath(path);
    if (inode_id == 0) return -1;

    // Update INode
    INode inode;
    inode_manager->get(inode_id, inode);
    inode.mode = mode;
    inode.ctime = time(NULL);
    inode_manager->set(inode_id, inode);
    return 0;
  }

  // int(* fuse_operations::chown) (const char *, uid_t, gid_t, struct fuse_file_info *fi)
  int fs_chown(const char* path, uid_t uid, gid_t gid) {
    debug("chown       %s to %d:%d\n", path, uid, gid);

    // Check if path exists
    INode::ID inode_id = file_access_manager->getINodeFromPath(path);
    if (inode_id == 0) return -1;

    // Update INode
    INode inode;
    inode_manager->get(inode_id, inode);
    inode.uid = uid;
    inode.gid = gid;
    inode.ctime = time(NULL);
    inode_manager->set(inode_id, inode);
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

    // Check if path exists
    INode::ID inode_id = file_access_manager->getINodeFromPath(path);
    if (inode_id == 0) return -1;

    // Read INode properties
    INode inode;
    inode_manager->get(inode_id, inode);
    info->st_atim.tv_sec = inode.atime;
    info->st_ctim.tv_sec = inode.ctime;
    info->st_mtim.tv_sec = inode.mtime;
    info->st_size = inode.size;
    info->st_blocks = inode.blocks;
    info->st_nlink = inode.links_count;
    info->st_gid = inode.gid;
    info->st_uid = inode.uid;
    info->st_mode = inode.mode;
    info->st_ino = inode_id;
    info->st_blksize = Block::SIZE;
    // info->st_dev = inode.dev;
    // info->st_rdev = inode.rdev;

    // Update INode properties
    inode.atime = time(NULL);
    inode.ctime = time(NULL);
    inode_manager->set(inode_id, inode);
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
  int fs_link(const char* oldpath, const char* newpath) {
    debug("link        %s -> %s\n", newpath, oldpath);

    // Check if newpath exists - if so, don't overwrite it
    INode::ID inode_id = file_access_manager->getINodeFromPath(newpath);
    if (inode_id != 0) return -1;

    // Check if oldpath exists
    inode_id = file_access_manager->getINodeFromPath(oldpath);
    if (inode_id == 0) return -1;

    // Get the newpath's directory
    std::string new_dname = file_access_manager->dirname(newpath);
    std::string new_fname = file_access_manager->basename(newpath);
    Directory dir = Directory::get(new_dname);

    // Write link to oldpath's inode in newpath's directory
    dir[new_fname] = inode_id;
    dir.save();

    // Update oldpath INode
    INode inode;
    inode_manager->get(inode_id, inode);
    inode.links_count += 1;
    inode.atime = time(NULL);
    inode.ctime = time(NULL);
    inode_manager->set(inode_id, inode);
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

    // Check if path exists - if so, don't overwrite it
    INode::ID inode_id = file_access_manager->getINodeFromPath(path);
    if (inode_id != 0) return -1;

    // Check if path's parent directory exists
    std::string parent_dname = file_access_manager->dirname(path);
    std::string dname = file_access_manager->basename(path);
    inode_id = file_access_manager->getINodeFromPath(parent_dname);
    if (inode_id == 0) return -1;

    // Update parent's directory
    INode parent_dir_inode;
    inode_manager->get(inode_id, parent_dir_inode);
    parent_dir_inode.mtime = time(NULL);
    parent_dir_inode.ctime = time(NULL);
    inode_manager->set(inode_id, parent_dir_inode);

    // Allocate an inode for new directory and write in parent directory
    INode::ID new_dir_inode_id = inode_manager->reserve();
    Directory dir = Directory::get(inode_id);
    dir[dname] = new_dir_inode_id;
    dir.save();

    // Set the new directory's attributes
    INode new_dir_inode;
    memset(&new_dir_inode, 0, sizeof(new_dir_inode));
    new_dir_inode.mode = mode;
    new_dir_inode.atime = time(NULL);
    new_dir_inode.mtime = time(NULL);
    new_dir_inode.ctime = time(NULL);
    new_dir_inode.type = FileType::DIRECTORY;
    new_dir_inode.blocks = 0;
    new_dir_inode.size = 0;
    new_dir_inode.links_count = 1;
    inode_manager->set(new_dir_inode_id, new_dir_inode);

    // TODO: How do we set these?
    // new_dir_inode.uid = ???
    // new_dir_inode.gid = ???
    // new_dir_inode.flags = ???

    return 0;
  }

  // int(* fuse_operations::mknod) (const char *, mode_t, dev_t)
  int fs_mknod(const char* path, mode_t mode, dev_t dev) {
    debug("mknod       %s %03o\n", path, mode);

    // Check if path exists - if so, don't overwrite it
    INode::ID inode_id = file_access_manager->getINodeFromPath(path);
    if (inode_id != 0) return -1;

    // Check if path's parent directory exists
    std::string parent_dname = file_access_manager->dirname(path);
    std::string fname = file_access_manager->basename(path);
    inode_id = file_access_manager->getINodeFromPath(parent_dname);
    if (inode_id == 0) return -1;

    // Update parent's directory
    INode parent_dir_inode;
    inode_manager->get(inode_id, parent_dir_inode);
    parent_dir_inode.mtime = time(NULL);
    parent_dir_inode.ctime = time(NULL);
    inode_manager->set(inode_id, parent_dir_inode);

    // Allocate an inode for new file and write in parent directory
    INode::ID new_file_inode_id = inode_manager->reserve();
    Directory dir = Directory::get(inode_id);
    dir[fname] = new_file_inode_id;
    dir.save();

    // Set the new file's attributes
    INode new_file_inode;
    memset(&new_file_inode, 0, sizeof(new_file_inode));
    new_file_inode.mode = mode;
    new_file_inode.atime = time(NULL);
    new_file_inode.mtime = time(NULL);
    new_file_inode.ctime = time(NULL);
    new_file_inode.type = FileType::REGULAR;
    new_file_inode.blocks = 0;
    new_file_inode.size = 0;
    new_file_inode.links_count = 1;
    inode_manager->set(new_file_inode_id, new_file_inode);

    // TODO: How do we set these?
    // new_file_inode.uid = ???
    // new_file_inode.gid = ???
    // new_file_inode.flags = ???
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

    // Check if file exists
    INode::ID inode_id = file_access_manager->getINodeFromPath(path);
    if (inode_id == 0) return -1;

    // Read data
    return file_access_manager->read(inode_id, buffer, size, offset);
  }

  // int(* fuse_operations::readlink) (const char *, char *, size_t)
  int fs_readlink(const char* path, char* buffer, size_t size) {
    debug("readlink    %s\n", path);

    INode::ID inode_id = file_access_manager->getINodeFromPath(path);
    INode inode;
    inode_manager->get(inode_id, inode);

    // Checking if the inode type is a SYMLINK
    if(inode.type != FileType::SYMLINK) return -1;

    // Need to get the string that symlink points to
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

    std::string pname = file_access_manager->dirname(path);
    std::string dname = file_access_manager->basename(path);

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

    //  Ignoring for now

    // this needs to be a struct that contains info about filesystem
    // such as number of free blocks, total blocks, type of file system etc.
    // doesn't seem useful here
    // int status = statvfs(path, info);
    // if(status == -1) {
    //   return -1;
    // }
    // TODO...
    return 0;
  }

  // int(* fuse_operations::symlink) (const char *, const char *)
  int fs_symlink(const char* path, const char* link) {
    debug("symlink     %s -> %s\n", path, link);

    std::string dname = file_access_manager->dirname(path);
    std::string fname = file_access_manager->basename(path);

    // INode::ID inode_id = inode_manager->reserve();
    // INode inode;
    // inode_manager->get(inode_id, inode);
    // inode.type = FileType::SYMLINK;
    // inode_manager->set(inode_id, inode);

    INode::ID inode_id = file_access_manager->getINodeFromPath(link);
    Directory dir = Directory::get(dname);
    // dir[fname].type = FileTypeDirectory::SYMLINK; //if we don't do this then we just won't know what type it is but will work
    dir[fname] = inode_id;
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

    std::string dname = file_access_manager->dirname(path);
    std::string fname = file_access_manager->basename(path);

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
    INode::ID inode_id = file_access_manager->getINodeFromPath(path);
    INode inode;
    inode_manager->get(inode_id, inode);

    // Update the times
    inode.time  = (uint32_t)buffer[0];  // update access time
    inode.mtime = (uint32_t)buffer[1];  // update modification time

    // Set the changes back to inode
    inode_manager->set(inode_id, inode);
    return 0;
  }

  // int(* fuse_operations::write) (const char *, const char *, size_t, off_t, struct fuse_file_info *)
  int fs_write(const char* path, const char* data, size_t size, off_t offset, fuse_file_info* info) {
    debug("write       %s %zdb at %zd\n", path, (int64_t) size, (int64_t) offset);
    return file_access_manager->write(path, data, size, offset);
  }

  // void*(* fuse_operations::init) (struct fuse_conn_info *conn, struct fuse_config *cfg)
  int fs_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
    // Useless function for us
    return NULL;
  }
}

int main(int argc, char** argv) {
  // added initialization for fuse arguments
  // can also be changed to 0, NULL for testing empty
  struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

  // Default ~32GB disk
  // TODO: Read value from argv
  uint64_t nblocks = 1 + 10 + (1 + 512) + (1 + 512 + 512*512) + (1 + 2 + 512*2 + 512*512*2);

  // Instantiate objects for filesystem
  disk = new MemoryStorage(nblocks);
  block_manager = new StackBasedBlockManager(*disk);
  inode_manager = new LinearINodeManager(*disk);
  file_access_manager = new FileAccessManager(*block_manager, *inode_manager, *disk);

  // Set FUSE function pointers
  fuse_operations ops;
  memset(&ops, 0, sizeof(ops));

  ops.chmod       = &fs_chmod;
  ops.chown       = &fs_chown;
  // ops.flush       = &fs_flush;
  // ops.fsync       = &fs_fsync;
  ops.getattr     = &fs_getattr;
  // ops.getxattr    = &fs_getxattr;
  ops.link        = &fs_link;
  // ops.listxattr   = &fs_listxattr;
  ops.mkdir       = &fs_mkdir;
  ops.mknod       = &fs_mknod;
  ops.open        = &fs_open;
  ops.read        = &fs_read;
  ops.readlink    = &fs_readlink;
  ops.release     = &fs_release;
  // ops.removexattr = &fs_removexattr;
  ops.rename      = &fs_rename;
  ops.rmdir       = &fs_rmdir;
  // ops.setxattr    = &fs_setxattr;
  // ops.statfs      = &fs_statfs;
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

  // Run the FUSE daemon!
  return fuse_main(argc, argv, &ops, NULL);
}
