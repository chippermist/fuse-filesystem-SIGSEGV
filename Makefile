BINARIES = mkfs fuse fsck
SOURCES  = $(shell find src/lib -name '*.cpp')
OBJECTS  = $(patsubst src/%.cpp, obj/%.o, $(SOURCES))

CXXFLAGS = -std=c++11 -Wall -Wextra
#LDFLAGS  = -lfuse

all: $(BINARIES)
mkfs: bin/mkfs
fuse: bin/fuse
fsck: bin/fsck

# Pattern for executables:
bin/%: obj/%.o $(OBJECTS)
	${CXX} $(CXXFLAGS) $(LDFLAGS) -o $@ $^

# Pattern for objects:
obj/%.o: src/%.cpp
	@test -d $(dir $@) || mkdir -p $(dir $@)
	${CXX} $(CXXFLAGS) -MMD -c -o $@ $<

clean:
	rm -rf bin/* obj/*

# Automatic dependencies:
-include $(OBJECTS:.o=.d)

# Don't remove the cached object files:
.SECONDARY: $(OBJECTS) $(patsubst %, obj/%.o, $(BINARIES))
