#include "directory_test.h"

// Compile as
// g++ -std=c++11 -DFUSE_USE_VERSION=26 -D_FILE_OFFSET_BITS=64 -D_DARWIN_USE_64_BIT_INODE -I/usr/local/include/osxfuse/fuse -losxfuse -L/usr/local/lib directory_test.cpp -o directory_test

ListINodeManager *inode_manager;
StackBasedBlockManager *block_manager;
Filesystem *filesystem;
int count = 0;

typedef std::vector<char> char_array;

char_array charset()
{
    return char_array( 
    {'0','1','2','3','4',
    '5','6','7','8','9',
    'A','B','C','D','E','F',
    'G','H','I','J','K',
    'L','M','N','O','P',
    'Q','R','S','T','U',
    'V','W','X','Y','Z',
    'a','b','c','d','e','f',
    'g','h','i','j','k',
    'l','m','n','o','p',
    'q','r','s','t','u',
    'v','w','x','y','z'
    });
};    

// given a function that generates a random character,
// return a string of the requested length
std::string random_string( size_t length, std::function<char(void)> rand_char )
{
    std::string str(length,0);
    std::generate_n( str.begin(), length, rand_char );
    return str;
}

void createNestedDirectories(Directory parent) {
  srand(time(NULL));
  int times = rand() % 50 + 1;
  if(count >= 1000) return; // exit condition

  for(int k=0; k<times; ++k) {
    INode::ID inode_id = inode_manager->reserve();
    INode inode;
    inode_manager->get(inode_id, inode);
    inode.type = FileType::DIRECTORY;
    filesystem->save(inode_id, inode);
    Directory new_dir = Directory(inode_id, parent.id());
    // std::cout << "Directory created: " << new_dir.id() << std::endl << std::endl;
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
  int times = rand() % 10 + 1;
  if(count >= 100) return;  //  exit condition

  for(int k=0; k<times; ++k) {
    srand(time(NULL));
    int name_length = rand() % 256 + 10;
    // generating random string through characterset
    const auto ch_set = charset();
    std::default_random_engine rng(std::random_device{}()); //trying not to use rand() 
    std::uniform_int_distribution<> dist(0, ch_set.size()-1);
    auto randchar = [ch_set, &dist, &rng](){return ch_set[dist(rng)];};
    std::string dir_name = random_string(name_length, randchar);

    // now doing all the standard mkdir stuff 
    INode::ID inode_id = inode_manager->reserve();
    INode inode;
    inode_manager->get(inode_id, inode);
    inode.type = FileType::DIRECTORY;
    filesystem->save(inode_id, inode);
    // std::cout << parent.id() << std::endl;
    Directory new_dir = Directory(inode_id, parent.id());
    parent.insert(dir_name, inode_id);
    filesystem->save(new_dir);
    filesystem->save(parent);
    //std::cout << "Directory created: " << new_dir.id() << "\nname: " << dir_name <<  std::endl << std::endl;

    // checking if . and .. are correct
    INode::ID self_id = new_dir.search(".");
    INode::ID parent_id = new_dir.search("..");

    parent = filesystem->getDirectory(parent.id());

    assert(self_id == inode_id);
    assert(parent_id == parent.id());

    // checking if the dir name was stored in the parent
    assert(parent.search(dir_name) != 0);
    assert(parent.contents().size() > 2); //checking if the entry was created
    
    ++count;
    createNamedNestedDirectories(new_dir);
    // showAllContents(parent);
    // assert(new_dir.contents().size() > 2); //checking if entry was created
  }
  // showAllContents(parent);
  // std::cout << "\n\n\n";
  return;
}

void showAllContents(Directory parent) {
  std::unordered_map<std::string, INode::ID> entries = parent.contents();
  for(const auto& it: entries) {
    std::cout << it.first << std::endl;
  }
  std::cout << std::endl << std::endl;
}


void deleteNestedDirectories(Directory parent) {
  //Directory dir = filesystem->getDirectory(parent.id());
  std::unordered_map<std::string, INode::ID> entries = parent.contents();
  std::string self = ".";
  std::string up_dir = "..";
  
  for(const auto& it : entries) {
    // showAllContents(parent);
    // std::cout << it.first << " ";
    if(it.first != self && it.first != up_dir) {
      // std::cout << "calling again\n";
      INode inode;
      inode_manager->get(it.second, inode);
      //checking if it's a directory
      assert(inode.type == FileType::DIRECTORY);
      Directory next_dir = filesystem->getDirectory(it.second);
      deleteNestedDirectories(next_dir);
      std::string entry_name = it.first;
      parent.remove(entry_name);
      inode_manager->release(it.second);
      filesystem->save(parent);
      // std::cout << "removed " << it.first << std::endl;
    }  
  }
  // only "." and ".."
  assert(parent.contents().size() == 2);

  return;
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
  inode_manager = new ListINodeManager(*disk);
  block_manager = new StackBasedBlockManager(*disk);
  filesystem = new Filesystem(*block_manager, *inode_manager, *disk);
  filesystem->mkfs();

  //------------------------------------------
  // Begining of test
  //------------------------------------------

  INode::ID inode_id_1 = inode_manager->reserve();
  std::cout << "\nRoot directory (\"/\") created: " << inode_id_1 << std::endl << std::endl;
  INode inode;
  inode_manager->get(inode_id_1, inode);
  inode.type = FileType::DIRECTORY;
  filesystem->save(inode_id_1, inode);
  Directory parent_dir(inode_id_1, inode_id_1);

  //naming the root as "/"
  // parent_dir.insert("/", inode_id_1);
  filesystem->save(parent_dir);
  ++count;

  // createNestedDirectories(parent_dir);
  // std::cout << "\n\n-------------\n";
  // std::cout << "\033[1;32mSuccess. End of create nested directory test.\033[0m\n";
  // std::cout << "-------------\n\n";
  // count = 0;
  createNamedNestedDirectories(parent_dir);
  std::cout << "\n\n-------------\n";
  std::cout << "\033[1;32mSuccess. End of create nested directory with name test.\033[0m\n";
  std::cout << "-------------\n\n";

  parent_dir = filesystem->getDirectory(inode_id_1);
  std::cout << "contents of root are \n";
  showAllContents(parent_dir);

  deleteNestedDirectories(parent_dir);
  std::cout << "\n\n-------------\n";
  std::cout << "\033[1;32mSuccess. End of delete nested directory with name test.\033[0m\n";
  std::cout << "-------------\n\n";

  // showAllContents(parent_dir);
  return 0;
}