#pragma once

#include "lib/Storage.h"
#include "lib/storage/MemoryStorage.h"
#include "lib/blocks/StackBasedBlockManager.h"
#include "lib/inodes/LinearINodeManager.h"
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

void createNestedDirectories(Directory);