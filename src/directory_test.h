#pragma once

#include "lib/Storage.h"
#include "lib/storage/MemoryStorage.h"
#include "lib/blocks/StackBasedBlockManager.h"
#include "lib/inodes/ListINodeManager.h"
#include "lib/Filesystem.h"
#include "lib/INode.h"
#include "lib/Block.h"
#include "lib/Directory.h"
#include <iostream>
#include <cstdint>
#include <stdexcept>
#include <cstring>
#include <cassert>
#include <stdlib.h>
#include <time.h>
#include <random>
#include <vector>
#include <unordered_map>
#include <functional>
#include <algorithm>

void createNestedDirectories(Directory );
void createNamedNestedDirectories(Directory );
void showAllContents(Directory );
void deleteNestedDirectories(Directory );
std::string random_string( size_t , std::function<char(void)>  );