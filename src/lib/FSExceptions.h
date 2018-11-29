#pragma once

#include <system_error>
#include <iostream>

  // Placeholder superclass for inheritence.
class FSException: public std::system_error {
protected:
  FSException(int code, const char* message): std::system_error(code, message) {}
};

struct AccessDenied: public FSException {
  // TODO: Alternative constructor that takes a path.
  AccessDenied(): FSException(EACCES, "Access denied!") {};
};

struct AlreadyExists: public FSException {
  // TODO: Alternative constructor that takes a path.
  AlreadyExists(): FSException(EEXIST, "File already exists!") {};
};

struct OutOfDataBlocks: public FSException {
  OutOfDataBlocks(): FSException(ENOSPC, "Out of data blocks!") {};
};

struct OutOfINodes: public FSException {
  OutOfINodes(): FSException(ENOSPC, "Out of INodes!") {};
};

struct NotADirectory: public FSException {
  // TODO: Alternative constructor that takes a path.
  NotADirectory(): FSException(ENOTDIR, "Not a directory!") {};
};

struct NoSuchFile: public FSException {
  // TODO: Alternative constructor that takes a path.
  NoSuchFile(): FSException(ENOENT, "No such file!") {};
};


int handle(std::function<int(void)> callback) {
  try {
    return callback();
  }
  catch(FSException& ex) {
    std::cerr << " ! " << ex.what() << '\n';
    return -ex.code().value();
  }
  catch(std::exception& ex) {
    std::cerr << " ! Unknown std::exception: " << ex.what() << '\n';
    return -1;
  }
  // catch(...) {
  //   // This will catch literally anything, so it can be dangerous...
  //   std::cerr << " ! BLEARRRGHHHHH!!!\n";
  //   return -1;
  // }
}
