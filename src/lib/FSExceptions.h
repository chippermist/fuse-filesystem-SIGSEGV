#pragma once

#include <system_error>
#include <iostream>

  // Placeholder superclass for inheritence.
class FSException: public std::system_error {
protected:
  // Pass this std::errc enums listed at https://en.cppreference.com/w/cpp/error/errc
  FSException(std::errc code, const char* message): std::system_error(std::make_error_code(code), message) {}
  FSException(std::errc code, const std::string& message): std::system_error(std::make_error_code(code), message) {}
};

struct AccessDenied: public FSException {
  // TODO: Alternative constructor that takes a path.
  AccessDenied(): FSException(std::errc::permission_denied, "Access denied!") {}
};

struct AlreadyExists: public FSException {
  // TODO: Alternative constructor that takes a path.
  AlreadyExists(): FSException(std::errc::file_exists, "File already exists!") {}
};

struct DirectoryNotEmpty: public FSException {
  // TODO: Alternative constructor that takes a path.
  DirectoryNotEmpty(): FSException(std::errc::directory_not_empty, "Directory not empty!") {}
};

struct IOError: public FSException {
  IOError(): FSException(std::errc::io_error, "IO error!") {}
  IOError(const std::string& message): FSException(std::errc::io_error, message) {}
};

struct OutOfDataBlocks: public FSException {
  OutOfDataBlocks(): FSException(std::errc::no_space_on_device, "Out of data blocks!") {}
};

struct OutOfINodes: public FSException {
  OutOfINodes(): FSException(std::errc::no_space_on_device, "Out of INodes!") {}
};

struct NotADirectory: public FSException {
  // TODO: Alternative constructor that takes a path.
  NotADirectory(): FSException(std::errc::not_a_directory, "Not a directory!") {}
};

struct NoSuchFile: public FSException {
  // TODO: Alternative constructor that takes a path.
  NoSuchFile(): FSException(std::errc::no_such_file_or_directory, "No such file!") {}
};


inline int handle(std::function<int(void)> callback) {
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
