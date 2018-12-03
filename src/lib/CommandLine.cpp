#include "Filesystem.h"

#include "blocks/StackBasedBlockManager.h"
#include "inodes/ListINodeManager.h"
#include "storage/MemoryStorage.h"
#include "storage/FileStorage.h"

#include <iostream>
#include <getopt.h>

static void usage(const char* message = NULL) {
  if(message) std::cerr << message << "\n\n";
  std::cerr << "USAGE: program [options] [mount-point]\n";
  std::cerr << "  --block-size  -b <num>  Block size (defaults to 4096).\n";
  std::cerr << "  --block-count -n <num>  Total number of blocks (mkfs only).\n";
  std::cerr << "  --inode-count -i <num>  Minimum number of INodes (mkfs only).\n";
  std::cerr << "  --disk-file   -f <str>  File or device to use for storage.\n";
  std::cerr << "  --debug       -d        Enable FUSE debugging output.\n";
  std::cerr << "  --parallel    -p        Run in multithreaded mode.\n";
  std::cerr << "  --quiet       -q        Reduce verbosity; may be repeated.\n";
  exit(1);
}

Filesystem::Filesystem(int argc, char** argv, bool mkfs) {
  char*    disk_file   = NULL;
  uint64_t block_size  = 4096;
  uint64_t block_count = 0;
  uint64_t inode_count = 0;

  mount_point = NULL;
  parallel    = false;
  debug       = false;
  verbosity   = 3;

  struct option options[] = {
    {"block-size",  required_argument, 0, 'b'},
    {"block-count", required_argument, 0, 'n'},
    {"inode-count", required_argument, 0, 'i'},
    {"disk-file",   required_argument, 0, 'f'},
    {"debug",             no_argument, 0, 'd'},
    {"parallel",          no_argument, 0, 'p'},
    {"quiet",             no_argument, 0, 'q'},
    {0, 0, 0, 0}
  };

  while(true) {
    int i = 0;
    int c = getopt_long(argc, argv, "b:n:i:f:dpq", options, &i);
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
    case 'd':
      debug = true;
      break;
    case 'p':
      parallel = true;
      break;
    case 'q':
      verbosity -= 1;
      break;
    default:
      std::cerr << "Unknown argument: " << argv[i] << '\n';
      exit(1);
    }
  }

  if(optind == argc - 1) {
    mount_point = argv[optind];
  }
  else if(optind > argc) {
    usage("Too many positional arguments.");
  }

  if(disk_file == NULL) {
    // We always need to initialize memory storage.
    mkfs = true;
  }

  if(block_size < 256) {
    usage("Block size must be at least 256 bytes.");
  }

  uint64_t n = block_size;
  while((n & 1) == 0) n >>= 1;
  if(n != 1) {
    usage("Block size must be a power of two.");
  }

  if(block_count == 0) {
    usage("Block count is a required argument.");
    // if(mkfs) {
    //   usage("Block count is required for mkfs.");
    // }
    // else {
    //   // TODO: Load the block count from disk!
    // }
  }
  // else if(!mkfs) {
  //   usage("Block count option is only valid for mkfs.");
  // }

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
    if(mkfs) {
      Block block;
      std::memset(block.data, 0, Block::SIZE);
      disk->set(block_count - 1, block);
    }
  }
  else {
    disk = new MemoryStorage(block_count);
  }

  uint64_t ipb   = Block::SIZE / sizeof(Block::ID);
  max_file_size  = INode::DIRECT_POINTERS;
  max_file_size += INode::SINGLE_INDIRECT_POINTERS * ipb;
  max_file_size += INode::DOUBLE_INDIRECT_POINTERS * ipb * ipb;
  max_file_size += INode::TRIPLE_INDIRECT_POINTERS * ipb * ipb * ipb;
  max_file_size *= Block::SIZE;

  inode_manager = new ListINodeManager(*disk);
  block_manager = new StackBasedBlockManager(*disk);
  if(mkfs) this->mkfs(block_count, inode_blocks);
}
