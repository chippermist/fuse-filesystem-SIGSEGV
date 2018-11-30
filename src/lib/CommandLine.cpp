#include "Filesystem.h"

#include "blocks/StackBasedBlockManager.h"
#include "inodes/LinearINodeManager.h"
#include "storage/MemoryStorage.h"
#include "storage/FileStorage.h"

#include <iostream>
#include <getopt.h>

static void usage(const char* message = NULL) {
  if(message) std::cerr << message << "\n\n";
  std::cerr << "--block-size  -b <num>  Block size (defaults to 4096).\n";
  std::cerr << "--block-count -n <num>  Total number of blocks (mkfs only).\n";
  std::cerr << "--inode-count -i <num>  Minimum number of INodes (mkfs only).\n";
  std::cerr << "--disk-file   -f <str>  File or device to use for storage.\n";
  exit(1);;
}

Filesystem* parse(int argc, char** argv, bool mkfs) {
  char*    disk_file   = NULL;
  uint64_t block_size  = 4096;
  uint64_t block_count = 0;
  uint64_t inode_count = 0;

  struct option options[] = {
    {"block-size",  required_argument, 0, 'b'},
    {"block-count", required_argument, 0, 'n'},
    {"inode-count", required_argument, 0, 'i'},
    {"disk-file",   required_argument, 0, 'f'},
    {0, 0, 0, 0}
  };

  while(true) {
    int i = 0;
    int c = getopt_long(argc, argv, "b:n:i:f:", options, &i);
    if(c == -1) break;

    switch(c) {
    case 'b':
      block_size = atoi(optarg);
      break;
    case 'n':
      block_count = atoi(optarg);
      break;
    case 'i':
      inode_count = atoi(optarg);
      break;
    case 'f':
      disk_file = optarg;
      break;
    default:
      std::cerr << "Unknown argument: " << argv[i] << '\n';
      exit(1);
    }
  }

  // We always need to initialize memory storage.
  if(disk_file == NULL) mkfs = true;

  if(block_size < 256) {
    usage("Block size must be at least 256 bytes.");
  }

  uint64_t n = block_size;
  while(n > 0) n <<= 1;
  if(n != 1) {
    usage("Block size must be a power of two.");
  }

  if(block_count == 0) {
    usage("Block count is a required argument.");
  }
  else if(!mkfs) {
    usage("Block count option is only valid for mkfs.");
  }

  uint64_t inode_blocks = 0;
  if(inode_count == 0) {
    inode_blocks = block_count / 10;
  }
  else if(mkfs) {
    uint64_t ipb = block_size / sizeof(INode::ID);
    inode_blocks = (inode_count + ipb - 1) / ipb;
  }
  else {
    usage("INode count option is only valid for mkfs.");
  }

  if(inode_blocks >= block_count / 2) {
    usage("Too many INode blocks.");
  }

  Storage* disk = NULL;
  if(disk_file != NULL) {
    disk = new FileStorage(disk_file, block_count);
  }
  else {
    disk = new MemoryStorage(block_count);
  }

  INodeManager* inodes     = new LinearINodeManager(*disk);
  BlockManager* blocks     = new StackBasedBlockManager(*disk);
  Filesystem*   filesystem = new Filesystem(*blocks, *inodes);

  if(mkfs) {
    filesystem->mkfs(block_count, inode_blocks);
  }

  return filesystem;
}
