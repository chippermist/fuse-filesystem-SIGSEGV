#include "directory_test.h"

// Compile as
//  g++ -std=c++11 -DFUSE_USE_VERSION=26 -D_FILE_OFFSET_BITS=64 -D_DARWIN_USE_64_BIT_INODE -I/usr/local/include/osxfuse/fuse -losxfuse -L/usr/local/lib directory_test.cpp -o directory_test

LinearINodeManager *inode_manager;
StackBasedBlockManager *block_manager;
Filesystem *filesystem;
int count = 0;

std::string random_string(size_t length) {
  srand(time(NULL));
    auto randchar = []() -> char {
        const char charset[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[ rand() % max_index  + 1];
    };
    std::string str(length,0);
    std::generate_n( str.begin(), length, randchar);
    return str;
}

void createNestedDirectories(Directory parent) {
  srand(time(NULL));
  int times = rand() % 10 + 1;
  if(count >= 1000) return;

  for(int k=0; k<times; ++k) {
    INode::ID inode_id = inode_manager->reserve();
    INode inode;
    inode_manager->get(inode_id, inode);
    inode.type = FileType::DIRECTORY;
    filesystem->save(inode_id, inode);
    Directory new_dir = Directory(inode_id, parent.id());
    std::cout << "Directory created: " << new_dir.id() << std::endl << std::endl;
    filesystem->save(new_dir);

    // checking if . and .. are correct
    INode::ID self_id = new_dir.search(".");
    INode::ID parent_id = new_dir.search("..");

    assert(self_id == inode_id);
    assert(parent_id == parent.id());

    ++count;
    createNestedDirectories(new_dir);
  }
  return;
}


void createNamedNestedDirectories(Directory parent) {
  srand(time(NULL));
  int times = rand() % 100 + 1;
  if(count >= 1000) return;

  for(int k=0; k<times; ++k) {
    srand(time(NULL));
    int name_length = rand() % 350 + 1;
    std::string dir_name = random_string(name_length);
    INode::ID inode_id = inode_manager->reserve();
    INode inode;
    inode_manager->get(inode_id, inode);
    inode.type = FileType::DIRECTORY;
    filesystem->save(inode_id, inode);
    Directory new_dir = Directory(inode_id, parent.id());
    parent.insert(dir_name, inode_id);
    filesystem->save(new_dir);
    filesystem->save(parent);
    std::cout << "Directory created: " << new_dir.id() << "\nname: " << dir_name <<  std::endl << std::endl;

    // checking if . and .. are correct
    INode::ID self_id = new_dir.search(".");
    INode::ID parent_id = new_dir.search("..");

    assert(self_id == inode_id);
    assert(parent_id == parent.id());

    // checking if the dir name was stored in the parent
    assert(parent.search(dir_name) != 0);

    ++count;
    createNamedNestedDirectories(new_dir);
  }
}


void deleteNestedDirectories(Directory parent) {
  Directory dir = filesystem->getDirectory(parent.id());

}

int main() {
  uint64_t nblocks = (1 + 10 + (1 + 512) + (1 + 512 + 512*512) + (1 + 2 + 512*2 + 512*512*2));
  MemoryStorage *disk = new MemoryStorage(1 + 10 + (1 + 512) + (1 + 512 + 512*512) + (1 + 2 + 512*2 + 512*512*2));    //788496
  // Get superblock and clear it out
  Block block;
  Superblock* superblock = (Superblock*) &block;
  disk->get(0, block);
  memset(&block, 0, Block::SIZE);

  // Set basic superblock parameters
  superblock->block_size = Block::SIZE;
  superblock->block_count = nblocks;

  // Have approximately 1 inode per 2048 bytes of disk space
  superblock->inode_block_start = 1;
  if (((nblocks * Block::SIZE) / 2048 * INode::SIZE) % Block::SIZE == 0) {
    superblock->inode_block_count = ((nblocks * Block::SIZE) / 2048 * INode::SIZE) / Block::SIZE;
  } else {
    superblock->inode_block_count = ((nblocks * Block::SIZE) / 2048 * INode::SIZE) / Block::SIZE + 1;
  }
  superblock->data_block_start = superblock->inode_block_start + superblock->inode_block_count;
  superblock->data_block_count = superblock->block_count - superblock->data_block_start;

  // Write superblock to disk
  disk->set(0, block);

  // Initialize managers and call mkfs
  inode_manager = new LinearINodeManager(*disk);
  block_manager = new StackBasedBlockManager(*disk);
  filesystem = new Filesystem(*block_manager, *inode_manager, *disk);
  filesystem->mkfs();

  //------------------------------------------
  // Begining of test
  //------------------------------------------

  INode::ID inode_id_1 = inode_manager->reserve();
  std::cout << "Root directory created: " << inode_id_1 << std::endl << std::endl;
  INode inode;
  inode_manager->get(inode_id_1, inode);
  inode.type = FileType::DIRECTORY;
  filesystem->save(inode_id_1, inode);
  Directory parent_dir(inode_id_1, inode_id_1);

  //naming the root as "/"
  parent_dir.insert("/", inode_id_1);
  filesystem->save(parent_dir);
  ++count;

  //createNestedDirectories(parent_dir);
  count = 0;
  createNamedNestedDirectories(parent_dir);




  return 0;
}