BINARIES = mkfs fuse fsck test-syscalls
SOURCES  = $(shell find src/lib -name '*.cpp')
OBJECTS  = $(patsubst src/%.cpp, obj/%.o, $(SOURCES))

CXXFLAGS  = -std=c++11 -g -Wall -Wextra
CXXFLAGS += -DFUSE_USE_VERSION=26
CXXFLAGS += -D_FILE_OFFSET_BITS=64

ifeq ($(shell uname -s), Darwin)
	# Root for OSXFUSE includes and libraries
	OSXFUSE_ROOT = /usr/local
	#OSXFUSE_ROOT = /opt/local

	INCLUDE_DIR = $(OSXFUSE_ROOT)/include/osxfuse/fuse
	LIBRARY_DIR = $(OSXFUSE_ROOT)/lib

	CXXFLAGS += -I$(INCLUDE_DIR)
	CXXFLAGS += -D_DARWIN_USE_64_BIT_INODE
	LDFLAGS   = -losxfuse -L$(LIBRARY_DIR)
else
	LDFLAGS   = -lfuse
endif


all: $(BINARIES)
mkfs: bin/mkfs
fuse: bin/fuse
fsck: bin/fsck

test-syscalls: bin/test-syscalls

# Pattern for executables:
bin/%: obj/%.o $(OBJECTS)
	${CXX} $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Pattern for objects:
obj/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	${CXX} $(CXXFLAGS) -MMD -c -o $@ $<

tests: $(BINARIES)
	@mkdir -p tmp/mnt
	bin/test tmp/mnt

clean:
	rm -rf obj tmp/tests $(patsubst %, bin/%, $(BINARIES))

# Automatic dependencies:
-include $(OBJECTS:.o=.d)

# Don't remove the cached object files:
.SECONDARY: $(OBJECTS) $(patsubst %, obj/%.o, $(BINARIES))
