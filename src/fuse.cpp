#include "lib/Filesystem.h"
#include "lib/FSExceptions.h"

#if defined(__linux__)
  #include <sys/statfs.h>
  #include <sys/vfs.h>
#endif

#include <cstring>
#include <cinttypes>
#include <fuse.h>

// Global Filesystem
Filesystem* fs;

#ifndef NDEBUG
  #include <cstdio>
  #define debug0(name, format, ...) if(fs->verbosity > 2) fprintf(stderr, "[\e[90m%-14s\e[0m]: " format "\n", name, __VA_ARGS__)
  #define debug1(name, format, ...) if(fs->verbosity > 1) fprintf(stderr, "[\e[32m%-14s\e[0m]: " format "\n", name, __VA_ARGS__)
  #define debug2(name, format, ...) if(fs->verbosity > 0) fprintf(stderr, "[\e[1;92m%-14s\e[0m]: " format "\n", name, __VA_ARGS__)
#else
  #define debug0(...)
  #define debug1(...)
  #define debug2(...)
#endif

#define UNUSED(x) ((void) (x))

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
  int   fs_access(const char *, int);

  int fs_access(const char *path, int mode) {
    debug1("access", "%s", path);
    return handle([=]{
      INode::ID id = fs->getINodeID(path);
      INode inode  = fs->getINode(id);

      struct fuse_context *context = fuse_get_context();

      // should root be allowed to access everyting?
      if((context->uid == 0) && (context->gid == 0)) {
        return 0;
      }
      // checking uid and gid as well so restrict access
      if((context->uid != inode.uid) || (context->gid != inode.gid)) {
        throw AccessDenied(path);
      }

      for(int i = 0, check_mode = mode; i<3; ++i) {
        if(i == 0 && (check_mode & 1)) {
          if(!(inode.mode & S_IXUSR)) {
            throw AccessDenied(path);
          }
        }
        else if(i == 1 && (check_mode & 1)) {
          if(!(inode.mode & S_IWUSR)) {
            throw AccessDenied(path);
          }
        }
        else if(i == 2 && (check_mode & 1)) {
          if(!(inode.mode & S_IRUSR)) {
            throw AccessDenied(path);
          }
        }
        check_mode >>= 1;
      }

      return 0;
    });
  }

  int fs_chmod(const char* path, mode_t mode) {
    debug2("chmod", "%s to %03o", path, mode);
    return handle([=]{
      INode::ID id = fs->getINodeID(path);
      INode inode  = fs->getINode(id);

      inode.ctime = time(NULL);
      inode.mode  = mode;
      fs->save(id, inode);
      return 0;
    });
  }

  int fs_chown(const char* path, uid_t uid, gid_t gid) {
    debug2("chown", "%s to %d:%d", path, uid, gid);
    return handle([=]{
      INode::ID id = fs->getINodeID(path);
      INode inode  = fs->getINode(id);

      if (uid != uid_t(0) - 1) inode.uid = uid;
      if (gid != gid_t(0) - 1) inode.gid = gid;
      if ((uid != 0xffff && uid != 0xffffffff) || (gid != 0xffff && gid != 0xffffffff)) {
        inode.ctime = time(NULL);
      }
      fs->save(id, inode);
      return 0;
    });
  }

  int fs_flush(const char* path, fuse_file_info* info) {
    debug1("flush", "%s", path);
    UNUSED(info);

    // TODO...
    return 0;
  }

  int fs_fsync(const char* path, int unknown, fuse_file_info* info) {
    debug1("fsync", "%s", path);
    UNUSED(unknown);
    UNUSED(info);

    // TODO...
    return 0;
  }

  int fs_getattr(const char* path, struct stat* info) {
    debug1("getattr", "%s", path);
    UNUSED(info);

    return handle([=]{
      INode::ID id = fs->getINodeID(path);
      INode inode  = fs->getINode(id);

      info->st_atime   = inode.atime;
      info->st_ctime   = inode.ctime;
      info->st_mtime   = inode.mtime;
      info->st_size    = inode.size;
      info->st_blocks  = inode.blocks;
      info->st_nlink   = inode.links;
      info->st_gid     = inode.gid;
      info->st_uid     = inode.uid;
      info->st_mode    = inode.mode;
      info->st_ino     = id;
      info->st_blksize = Block::SIZE;
      info->st_dev     = inode.dev;
      // info->st_rdev = inode.rdev;

      // Modify mode depending on file type
      if (inode.type == FileType::REGULAR) {
        info->st_mode = info->st_mode | S_IFREG;
      } else if (inode.type == FileType::DIRECTORY) {
        info->st_mode = info->st_mode | S_IFDIR;
      } else if (inode.type == FileType::SYMLINK) {
        info->st_mode = info->st_mode | S_IFLNK;
      } else {
        throw std::out_of_range("Called getattr on a free INode!");
      }
      return 0;
    });
  }

  int fs_getxattr(const char* path, const char* attr, char* buffer, size_t size) {
    debug1("getxattr", "%s %s", path, attr);
    UNUSED(buffer);
    UNUSED(size);

    // Not implemented!
    return -1;
  }

  void* fs_init(struct fuse_conn_info* conn) {
    debug2("init", "%p", conn);
    UNUSED(conn);

    // Useless function for us
    return NULL;
  }

  int fs_link(const char* target, const char* link) {
    debug2("link", "%s -> %s", link, target);
    return handle([=]{
      INode::ID id = fs->getINodeID(target);
      INode inode  = fs->getINode(id);

      std::string dname = fs->dirname(link);
      std::string fname = fs->basename(link);

      // Get the link's directory
      Directory dir = fs->getDirectory(dname);
      if(dir.contains(fname)) {
        throw AlreadyExists(link);
      }

      inode.ctime = time(NULL);
      inode.links += 1;
      fs->save(id, inode);

      dir.insert(fname, id);
      fs->save(dir);
      return 0;
    });
  }

  int fs_listxattr(const char* path, char* buffer, size_t size) {
    debug1("listxattr", "%s", path);
    UNUSED(buffer);
    UNUSED(size);

    // Not implemented!
    return -1;
  }

  int fs_mkdir(const char* path, mode_t mode) {
    debug2("mkdir", "%s %03o", path, mode);
    return handle([=]{
      std::string pname = fs->dirname(path);
      std::string dname = fs->basename(path);

      Directory parent = fs->getDirectory(pname);
      if(parent.contains(dname)) {
        throw AlreadyExists(path);
      }

      INode::ID id = fs->newINodeID();
      INode inode(FileType::DIRECTORY, mode);
      fs->save(id, inode);

      Directory dir(id, parent.id());
      fs->save(dir);

      parent.insert(dname, id);
      fs->save(parent);
      return 0;
    });
  }

  int fs_mknod(const char* path, mode_t mode, dev_t dev) {
    debug2("mknod", "%s %03o", path, mode);

    if (!S_ISREG(mode)) {
      return -ENOTSUP;
    }

    return handle([=]{
      std::string dname = fs->dirname(path);
      std::string fname = fs->basename(path);

      Directory parent = fs->getDirectory(dname);
      if(parent.contains(dname)) {
        throw AlreadyExists(path);
      }

      INode::ID id = fs->newINodeID();
      INode inode(FileType::REGULAR, mode, dev);
      fs->save(id, inode);

      parent.insert(fname, id);
      fs->save(parent);
      return 0;
    });
  }

  int fs_open(const char* path, fuse_file_info* info) {
    debug1("open", "%s", path);
    UNUSED(info);

    // TODO...
    return 0;
  }

  int fs_read(const char* path, char* buffer, size_t size, off_t offset, fuse_file_info* info) {
    debug2("read", "%s %" PRIu64 "b at %" PRId64, path, (uint64_t) size, offset);
    UNUSED(info);

    return handle([=]{
      INode::ID id = fs->getINodeID(path);
      INode inode  = fs->getINode(id);
      if(inode.type != FileType::REGULAR) {
        throw NotAFile(path);
      }

      return fs->read(id, buffer, size, offset);
    });
  }

  int fs_readdir(const char* path, void* buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* info) {
    debug2("readdir", "%s", path);
    UNUSED(offset);
    UNUSED(info);

    return handle([=]{
      Directory dir = fs->getDirectory(path);
      for(const auto itr: dir.entries()) {
        int result = filler(buffer, itr.first.c_str(), NULL, 0);
        if(result != 0) return result;
      }

      return 0;
    });
  }

  int fs_readlink(const char* path, char* buffer, size_t size) {
    debug2("readlink", "%s", path);
    return handle([=]{
      INode::ID id = fs->getINodeID(path);
      INode inode  = fs->getINode(id);
      if(inode.type != FileType::SYMLINK) {
        throw NotASymlink(path);
      }

      fs->read(id, buffer, size, 0);
      return 0;
    });
  }

  int fs_release(const char* path, fuse_file_info* info) {
    debug1("release", "%s", path);
    UNUSED(info);

    // TODO...
    return 0;
  }

  int fs_removexattr(const char* path, const char* attr) {
    debug1("removexattr", "%s %s", path, attr);

    // Not implemented!
    return -1;
  }

  int fs_rename(const char* oldname, const char* newname) {
    debug2("rename", "%s -> %s", oldname, newname);
    return handle([=]{
      std::string dname = fs->dirname(newname);
      std::string fname = fs->basename(newname);

      // Stuff for the INode we're moving:
      INode::ID id = fs->getINodeID(oldname);
      INode inode  = fs->getINode(id);

      // Stuff for the INode we might be replacing:
      Directory newparent = fs->getDirectory(dname);
      INode::ID clobber   = newparent.search(fname);

      if(clobber != 0) {
        INode clobnode = fs->getINode(clobber);
        if(clobnode.type == FileType::DIRECTORY) {
          if(inode.type != FileType::DIRECTORY) {
            // [EISDIR]  New is a directory, but old is not a directory.
            throw IsADirectory(newname);
          }

          Directory clobdir = fs->getDirectory(clobber);
          if(!clobdir.isEmpty()) {
            // [ENOTEMPTY]  New is a directory and is not empty.
            throw DirectoryNotEmpty(newname);
          }
        }
        else if(inode.type == FileType::DIRECTORY) {
          // [ENOTDIR]  Old is a directory, but new is not a directory.
          throw NotADirectory(newname);
        }
      }

      inode.links += 1;
      inode.ctime  = time(NULL);
      fs->save(id, inode);

      newparent.insert(fname, id);
      fs->save(newparent);

      // Unlink all the old paths:
      if (clobber != 0) fs->unlink(clobber);
      return fs_unlink(oldname);
    });
  }

  int fs_rmdir(const char* path) {
    debug2("rmdir", "%s", path);
    return handle([=]{
      std::string pname = fs->dirname(path);
      std::string dname = fs->basename(path);

      Directory parent = fs->getDirectory(pname);
      INode::ID id     = parent.search(dname);
      Directory dir    = fs->getDirectory(id);
      if(!dir.isEmpty()) throw DirectoryNotEmpty(path);

      parent.remove(dname);
      fs->save(parent);

      fs->unlink(id);
      return 0;
    });
  }

  int fs_setxattr(const char* path, const char* attr, const char* val, size_t size, int unknown) {
    debug1("setxattr", "%s %s", path, attr);
    UNUSED(val);
    UNUSED(size);
    UNUSED(unknown);

    // Not implemented!
    return -1;
  }

  int fs_statfs(const char* path, struct statvfs* info) {
    debug1("statfs", "%s", path);
    return handle([=]{
      fs->statfs(info);
      return 0;
    });
  }

  int fs_symlink(const char* target, const char* link) {
    debug2("symlink", "%s -> %s", link, target);
    return handle([=]{
      std::string dname = fs->dirname(link);
      std::string fname = fs->basename(link);

      Directory dir = fs->getDirectory(dname);
      if(dir.contains(fname)) {
        throw AlreadyExists(link);
      }

      INode::ID id = fs->newINodeID();
      INode inode(FileType::SYMLINK, 0777);
      fs->save(id, inode);

      fs->write(id, target, std::strlen(target) + 1, 0);

      dir.insert(fname, id);
      fs->save(dir);
      return 0;
    });
  }

  int fs_truncate(const char* path, off_t offset) {
    debug2("truncate", "%s to %" PRId64 "b", path, (int64_t) offset);
    return handle([=]{
      INode::ID id = fs->getINodeID(path);
      INode inode  = fs->getINode(id);
      if(inode.type != FileType::REGULAR) {
        throw NotAFile(path);
      }

      return fs->truncate(id, offset);
    });
  }

  int fs_unlink(const char* path) {
    debug2("unlink", "%s", path);
    return handle([=]{
      std::string dname = fs->dirname(path);
      std::string fname = fs->basename(path);

      Directory dir = fs->getDirectory(dname);
      INode::ID fid = dir.search(fname);

      dir.remove(fname);
      fs->save(dir);

      fs->unlink(fid);
      return 0;
    });
  }

  // int(* fuse_operations::utimens) (const char *, const struct timespec tv[2], struct fuse_file_info *fi)
  // This supersedes the old utime() interface. New applications should use this.
  int fs_utime(const char* path, utimbuf* buffer) {
    debug2("utime", "%s", path);
    return handle([=]{
      INode::ID id = fs->getINodeID(path);
      INode inode = fs->getINode(id);
      inode.ctime = time(NULL);
      if(buffer->actime  == 0) buffer->actime  = inode.ctime;
      if(buffer->modtime == 0) buffer->modtime = inode.ctime;
      inode.atime = buffer->actime;
      inode.mtime = buffer->modtime;
      fs->save(id, inode);
      return 0;
    });
  }

  int fs_write(const char* path, const char* data, size_t size, off_t offset, fuse_file_info* info) {
    debug2("write", "%s %" PRIu64 "b at %" PRId64, path, (uint64_t) size, offset);
    UNUSED(info);

    return handle([=]{
      INode::ID id = fs->getINodeID(path);
      INode inode  = fs->getINode(id);
      if(inode.type != FileType::REGULAR) {
        throw NotAFile(path);
      }

      return fs->write(id, data, size, offset);
    });
  }
}

int main(int argc, char** argv) {
  fs = new Filesystem(argc, argv, false);
  // TODO: Make sure FS config matches!

  fuse_operations ops;
  memset(&ops, 0, sizeof(ops));

  // ops.access      = &fs_access;
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

  return fs->mount(argv[0], &ops);
}

/*
  References:
  1. relatime
    - http://wisercoder.com/knowing-difference-mtime-ctime-atime/
    - https://superuser.com/questions/464290/why-is-cat-not-changing-the-access-time
*/
