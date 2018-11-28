#include "lib/Filesystem.h"

#if defined(__linux__)
  #include <sys/statfs.h>
  #include <sys/vfs.h>
#endif

#include <fuse.h>
#include <cstring>

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
  // Adding declarations to resolve linker errors
  int fs_chmod(const char*, mode_t);
  int fs_chown(const char* , uid_t , gid_t );
  int fs_flush(const char* , fuse_file_info* );
  int fs_fsync(const char* , int , fuse_file_info* );
  int fs_getattr(const char* , struct stat* );
  int fs_getdir(const char* , fuse_dirh_t , fuse_dirfil_t );
  int fs_getxattr(const char* , const char* , char* , size_t );
  int fs_link(const char* , const char* );
  int fs_listxattr(const char* , char* , size_t );
  int fs_mkdir(const char* , mode_t );
  int fs_mknod(const char* , mode_t , dev_t );
  int fs_open(const char* , fuse_file_info* );
  int fs_read(const char* , char* , size_t , off_t , fuse_file_info* );
  int fs_readlink(const char* , char* , size_t );
  int fs_release(const char* , fuse_file_info* );
  int fs_removexattr(const char* , const char* );
  int fs_rename(const char* , const char* );
  int fs_rmdir(const char* );
  int fs_setxattr(const char* , const char* , const char* , size_t , int );
  int fs_statfs(const char* , struct statvfs* );
  int fs_symlink(const char* , const char* );
  int fs_truncate(const char* , off_t );
  int fs_unlink(const char* );
  int fs_utime(const char* , utimbuf* );
  int fs_write(const char* , const char* , size_t , off_t , fuse_file_info* );
  void* fs_init(struct fuse_conn_info *, struct fuse_config *);

  // int(* fuse_operations::chmod) (const char *, mode_t, struct fuse_file_info *fi)
  int fs_chmod(const char* path, mode_t mode) {
    debug("chmod       %s to %03o\n", path, mode);

    // Check if path exists
    INode::ID inode_id = file_access_manager->getINodeID(path);
    if (inode_id == 0) return -1;

    // Update INode
    INode inode;
    inode_manager->get(inode_id, inode);
    inode.mode = mode;
    inode.ctime = time(NULL);
    inode.atime = inode.ctime;
    inode_manager->set(inode_id, inode);
    return 0;
  }

  // int(* fuse_operations::chown) (const char *, uid_t, gid_t, struct fuse_file_info *fi)
  int fs_chown(const char* path, uid_t uid, gid_t gid) {
    debug("chown       %s to %d:%d\n", path, uid, gid);

    // Check if path exists
    INode::ID inode_id = file_access_manager->getINodeID(path);
    if (inode_id == 0) return -1;

    // Update INode
    INode inode;
    inode_manager->get(inode_id, inode);
    inode.uid = uid;
    inode.gid = gid;
    inode.ctime = time(NULL);
    inode.atime = inode.ctime;
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
    INode::ID inode_id = file_access_manager->getINodeID(path);
    if (inode_id == 0) return -1;

    // Read INode properties
    INode inode;
    inode_manager->get(inode_id, inode);
    info->st_atime = inode.atime;
    info->st_ctime = inode.ctime;
    info->st_mtime = inode.mtime;
    info->st_size = inode.size;
    info->st_blocks = inode.blocks;
    info->st_nlink = inode.links_count;
    info->st_gid = inode.gid;
    info->st_uid = inode.uid;
    info->st_mode = inode.mode;
    info->st_ino = inode_id;
    info->st_blksize = Block::SIZE;
    info->st_dev = inode.dev;
    // info->st_rdev = inode.rdev;

    // mount relatime - don't update atime
    // inode.atime = time(NULL);
    // inode_manager->set(inode_id, inode);
    return 0;
  }

  // did not find a definition in fuse::operations
  int fs_getdir(const char* path, fuse_dirh_t dirh, fuse_dirfil_t dirfil) {
    debug("getdir      %s\n", path);

    Directory dir = file_access_manager->getDirectory(path);
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
    if (file_access_manager->getINodeID(newpath) != 0) return -1;

    // Check if oldpath exists
    INode::ID inode_id = file_access_manager->getINodeID(oldpath);
    if (inode_id == 0) return -1;

    // Get the newpath's directory
    std::string new_dname = file_access_manager->dirname(newpath);
    std::string new_fname = file_access_manager->basename(newpath);
    Directory dir = file_access_manager->getDirectory(new_dname);

    // Write link to oldpath's inode in newpath's directory
    dir.insert(new_fname, inode_id);
    file_access_manager->save(dir);

    // Update oldpath INode's links count
    INode inode;
    inode_manager->get(inode_id, inode);
    inode.links_count += 1;
    inode.ctime = time(NULL);
    inode.atime = inode.ctime;
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
    if (file_access_manager->getINodeID(path) != 0) return -1;

    // Check if path's parent directory exists
    std::string parent_dname = file_access_manager->dirname(path);
    std::string dname = file_access_manager->basename(path);
    INode::ID parent_dir_id = file_access_manager->getINodeID(parent_dname);
    if (parent_dir_id == 0) return -1;

    // Allocate an inode for new directory and write in parent directory
    INode::ID new_dir_inode_id = inode_manager->reserve();
    Directory dir = file_access_manager->getDirectory(parent_dir_id);
    dir.insert(dname, new_dir_inode_id);
    file_access_manager->save(dir);

    // Set the new directory's attributes
    INode new_dir_inode;
    memset(&new_dir_inode, 0, sizeof(new_dir_inode));
    new_dir_inode.mode = mode;
    new_dir_inode.atime = time(NULL);
    new_dir_inode.mtime = new_dir_inode.atime;
    new_dir_inode.ctime = new_dir_inode.atime;
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
    if (file_access_manager->getINodeID(path) != 0) return -1;

    // Check if path's parent directory exists
    std::string parent_dname = file_access_manager->dirname(path);
    std::string fname = file_access_manager->basename(path);
    INode::ID parent_inode_id = file_access_manager->getINodeID(parent_dname);
    if (parent_inode_id == 0) return -1;

    // Allocate an inode for new file and write in parent directory
    INode::ID new_file_inode_id = inode_manager->reserve();
    Directory dir = file_access_manager->getDirectory(parent_inode_id);
    dir.insert(fname, new_file_inode_id);
    file_access_manager->save(dir);

    // Set the new file's attributes
    INode new_file_inode;
    memset(&new_file_inode, 0, sizeof(new_file_inode));
    new_file_inode.mode = mode;
    new_file_inode.atime = time(NULL);
    new_file_inode.mtime = new_file_inode.atime;
    new_file_inode.ctime = new_file_inode.atime;
    new_file_inode.type = FileType::REGULAR;
    new_file_inode.blocks = 0;
    new_file_inode.size = 0;
    new_file_inode.links_count = 1;
    new_file_inode.dev = dev;
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
    INode::ID inode_id = file_access_manager->getINodeID(path);
    if (inode_id == 0) return -1;

    // Read data
    return file_access_manager->read(inode_id, buffer, size, offset);
  }

  // int(* fuse_operations::readlink) (const char *, char *, size_t)
  int fs_readlink(const char* path, char* buffer, size_t size) {
    debug("readlink    %s\n", path);

    // Check if file exists
    INode::ID inode_id = file_access_manager->getINodeID(path);
    if (inode_id == 0) return -1;

    // TODO: Read symlink value
    INode file_inode;
    inode_manager->get(inode_id, file_inode);
    if (file_inode.type != FileType::SYMLINK) return -1;


    // mount relatime - don't update atime
    // file_inode.atime = time(NULL);
    // inode_manager->set(inode_id, file_inode);
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
    if (result < 0) return result;
    return fs_unlink(path);
  }

  // int(* fuse_operations::rmdir) (const char *)
  int fs_rmdir(const char* path) {
    debug("rmdir       %s\n", path);

    // Check if path exists
    if (file_access_manager->getINodeID(path) == 0) return -1;

    // Read parent directory's INode
    std::string pname = file_access_manager->dirname(path);
    std::string dname = file_access_manager->basename(path);
    INode::ID parent_inode_id = file_access_manager->getINodeID(pname);
    INode parent_dir_inode;
    inode_manager->get(parent_inode_id, parent_dir_inode);

    // Remove entry from parent directory
    Directory parent = file_access_manager->getDirectory(parent_inode_id);
    parent.remove(dname);
    file_access_manager->save(parent);
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

    INode::ID inode_id = file_access_manager->getINodeID(link);
    Directory dir = file_access_manager->getDirectory(dname);
    // dir[fname].type = FileTypeDirectory::SYMLINK; //if we don't do this then we just won't know what type it is but will work
    dir.insert(fname, inode_id);
    file_access_manager->save(dir);

    return 0;
  }

  // int(* fuse_operations::truncate) (const char *, off_t, struct fuse_file_info *fi)
  int fs_truncate(const char* path, off_t offset) {
    debug("truncate    %s to %zdb\n", path, (int64_t) offset);

    // Check if file exists
    INode::ID inode_id = file_access_manager->getINodeID(path);
    if (inode_id == 0) return -1;

    // Cut data
    return file_access_manager->truncate(inode_id, offset);
  }

  // int(* fuse_operations::unlink) (const char *)
  int fs_unlink(const char* path) {
    debug("unlink      %s\n", path);

    // Check if path exists
    if (file_access_manager->getINodeID(path) == 0) return -1;

    // Get parent directory's INode ID
    std::string pname = file_access_manager->dirname(path);
    std::string dname = file_access_manager->basename(path);
    INode::ID parent_inode_id = file_access_manager->getINodeID(pname);

    // Remove entry from parent directory
    Directory parent = file_access_manager->getDirectory(parent_inode_id);
    parent.remove(dname);
    file_access_manager->save(parent);
    return 0;
  }

  // int(* fuse_operations::utimens) (const char *, const struct timespec tv[2], struct fuse_file_info *fi)
  // This supersedes the old utime() interface. New applications should use this.
  int fs_utime(const char* path, utimbuf* buffer) {
    debug("utime       %s\n", path);

    // Check if path exists
    INode::ID inode_id = file_access_manager->getINodeID(path);
    if (inode_id == 0) return -1;
    INode inode;
    inode_manager->get(inode_id, inode);

    // Update INode
    if (buffer->actime == NULL) buffer->actime = time(NULL);
    if (buffer->modtime == NULL) buffer->modtime = time(NULL);
    inode.atime = buffer->actime;
    inode.mtime = buffer->modtime;
    inode.ctime = time(NULL);
    inode_manager->set(inode_id, inode);
    return 0;
  }

  // int(* fuse_operations::write) (const char *, const char *, size_t, off_t, struct fuse_file_info *)
  int fs_write(const char* path, const char* data, size_t size, off_t offset, fuse_file_info* info) {
    debug("write       %s %zdb at %zd\n", path, (int64_t) size, (int64_t) offset);

    // Check if file exists
    INode::ID inode_id = file_access_manager->getINodeID(path);
    if (inode_id == 0) return -1;

    // Write data
    return file_access_manager->write(inode_id, data, size, offset);
  }

  // void*(* fuse_operations::init) (struct fuse_conn_info *conn, struct fuse_config *cfg)
  void* fs_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
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

/*
  References:
  1. relatime
    - http://wisercoder.com/knowing-difference-mtime-ctime-atime/
    - https://superuser.com/questions/464290/why-is-cat-not-changing-the-access-time
*/
