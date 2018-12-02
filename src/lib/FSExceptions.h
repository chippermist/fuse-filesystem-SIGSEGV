#pragma once

#include <functional>
#include <iostream>
#include <system_error>

  // Placeholder superclass for inheritence.
class FSException: public std::system_error {
protected:
  // Pass this std::errc enums listed at https://en.cppreference.com/w/cpp/error/errc
  FSException(std::errc code, const char* message): std::system_error(std::make_error_code(code), message) {}
  FSException(std::errc code, const std::string& message): std::system_error(std::make_error_code(code), message) {}
};


struct AccessDenied: public FSException {
  AccessDenied(): FSException(std::errc::permission_denied, "Access denied!") {}
  AccessDenied(const std::string& path): FSException(std::errc::permission_denied, "Access denied: " + path) {}
};

struct AlreadyExists: public FSException {
  AlreadyExists(): FSException(std::errc::file_exists, "That already exists!") {}
  AlreadyExists(const std::string& path): FSException(std::errc::file_exists, "Already exists: " + path) {}
};

struct DirectoryNotEmpty: public FSException {
  DirectoryNotEmpty(): FSException(std::errc::directory_not_empty, "Directory not empty!") {}
  DirectoryNotEmpty(const std::string& path): FSException(std::errc::directory_not_empty, "Directory not empty: " + path) {}
};

struct FileTooBig: public FSException {
  FileTooBig(): FSException(std::errc::file_too_large, "File too big!") {}
  FileTooBig(const std::string& path): FSException(std::errc::file_too_large, "File too big: " + path) {}
};

struct IOError: public FSException {
  IOError(): FSException(std::errc::io_error, "IO error!") {}
  IOError(const std::string& message): FSException(std::errc::io_error, message) {}
};

struct IsADirectory: public FSException {
  IsADirectory(): FSException(std::errc::is_a_directory, "That's a directory!") {}
  IsADirectory(const std::string& path): FSException(std::errc::is_a_directory, "Directory: " + path) {}
};

struct OutOfDataBlocks: public FSException {
  OutOfDataBlocks(): FSException(std::errc::no_space_on_device, "Out of data blocks!") {}
};

struct OutOfINodes: public FSException {
  OutOfINodes(): FSException(std::errc::no_space_on_device, "Out of INodes!") {}
};

struct NotADirectory: public FSException {
  NotADirectory(): FSException(std::errc::not_a_directory, "That's not a directory!") {}
  NotADirectory(const std::string& path): FSException(std::errc::not_a_directory, "Not a directory: " + path) {}
};

struct NotAFile: public FSException {
  NotAFile(): FSException(std::errc::not_a_directory, "That's not a file!") {}
  NotAFile(const std::string& path): FSException(std::errc::invalid_argument, "Not a file: " + path) {}
};

struct NotASymlink: public FSException {
  NotASymlink(): FSException(std::errc::not_a_directory, "That's not a symlink!") {}
  NotASymlink(const std::string& path): FSException(std::errc::invalid_argument, "Not a symlink: " + path) {}
};

struct NoSuchEntry: public FSException {
  NoSuchEntry(): FSException(std::errc::no_such_file_or_directory, "No such entry!") {}
  NoSuchEntry(const std::string& path): FSException(std::errc::no_such_file_or_directory, "No such entry: " + path) {}
};


inline int handle(std::function<int(void)> callback) {
  try {
    return callback();
  }
  catch(NoSuchEntry& ex) {
    std::cerr << "[\e[2;33mnot found     \e[0m]: " << ex.what() << '\n';
    return -ex.code().value();
  }
  catch(FSException& ex) {
    std::cerr << "[\e[33mfs exception  \e[0m]: " << ex.what() << '\n';
    return -ex.code().value();
  }
  catch(std::exception& ex) {
    std::cerr << "[\e[1;31msystem error  \e[0m]: " << ex.what() << '\n';
    return -222; // Hopefully an unknown error code (-1 is access denied).
  }
  // catch(...) {
  //   // This will catch literally anything, so it can be dangerous...
  //   std::cerr << " ! BLEARRRGHHHHH!!!\n";
  //   return -1;
  // }
}
