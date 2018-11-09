BINARIES = mkfs fuse fsck
SOURCES  = $(shell find src/lib -name '*.cpp')
OBJECTS  = $(patsubst src/%.cpp, obj/%.o, $(SOURCES))


UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	# Root for OSXFUSE includes and libraries
	OSXFUSE_ROOT = /usr/local
	#OSXFUSE_ROOT = /opt/local

	INCLUDE_DIR = $(OSXFUSE_ROOT)/include/osxfuse/fuse
	LIBRARY_DIR = $(OSXFUSE_ROOT)/lib


	CXXFLAGS_OSXFUSE = -I$(INCLUDE_DIR) -L$(LIBRARY_DIR)
	CXXFLAGS_OSXFUSE += -DFUSE_USE_VERSION=26
	CXXFLAGS_OSXFUSE += -D_FILE_OFFSET_BITS=64
	CXXFLAGS_OSXFUSE += -D_DARWIN_USE_64_BIT_INODE
	LIBS = -losxfuse
endif

CXXFLAGS = -std=c++11 -Wall -Wextra -D_FILE_OFFSET_BITS=64

#LDFLAGS  = -lfuse

all: $(BINARIES)
mkfs: bin/mkfs
fuse: bin/fuse
fsck: bin/fsck

# Pattern for executables:
bin/%: obj/%.o $(OBJECTS)
	${CXX} $(CXXFLAGS) $(CXXFLAGS_OSXFUSE) $(LDFLAGS) -o $@ $^

# Pattern for objects:
obj/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	${CXX} $(CXXFLAGS) $(CXXFLAGS_OSXFUSE) -MMD -c -o $@ $<

clean:
	rm -rf bin/* obj/*

# Automatic dependencies:
-include $(OBJECTS:.o=.d)

# Don't remove the cached object files:
.SECONDARY: $(OBJECTS) $(patsubst %, obj/%.o, $(BINARIES))
