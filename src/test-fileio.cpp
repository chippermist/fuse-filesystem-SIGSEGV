#include <cassert>
#include <cerrno>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <fcntl.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>

#define min(a, b) ((a) < (b)? (a) : (b))
#define max(a, b) ((a) > (b)? (a) : (b))

const int64_t MAX_CALL = 1024 * 1024;
const int64_t MAX_SIZE = MAX_CALL * 2 + 2048;
const char    zeros[1024] = {0};


// Global variables are evil.
int64_t filesize   = 0;
int64_t fileoffset = 0;
char    filedata[MAX_SIZE];
bool    verbose = false;

int64_t n_reads  = 0;
int64_t n_writes = 0;
int64_t n_exts   = 0;
int64_t n_chops  = 0;

int64_t bytes_read     = 0;
int64_t bytes_written  = 0;
int64_t bytes_chopped  = 0;
int64_t bytes_extended = 0;


int64_t getusec() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return 1000000 * tv.tv_sec + tv.tv_usec;
}

int64_t randomize(int64_t max, int64_t align) {
  assert(max <= RAND_MAX);
  return (rand() % max / align) * align;
}

void randbytes(char* buffer, int64_t size) {
  for(int i = 0; i < size; ++i) {
    buffer[i] = rand();
  }
}

int openfile(const char* file, int mode) {
  int fd = open(file, mode);
  if(fd <= 0) {
    fprintf(stderr, "FILE OPEN FAIL: %d\n", errno);
    exit(1);
  }

  return fd;
}

void read(const char* file, char* data, int64_t length, int64_t offset) {
  if(verbose) printf("READ:  %8" PRId64 " bytes at %8" PRId64 "\n", length, offset);
  int64_t len = min(filesize - offset, length);
  len = max(len, 0);

  int fd = openfile(file, O_RDONLY);
  int64_t result = pread(fd, data, length, fileoffset + offset);
  close(fd);

  if(result != len) {
    fprintf(stderr, "READ FAIL (%" PRId64 " != %" PRId64 "): %d\n", result, len, errno);
    fprintf(stderr, "Read Offset: %" PRId64 "\n", offset);
    fprintf(stderr, "Read Length: %" PRId64 "\n", length);
    fprintf(stderr, "File Length: %" PRId64 "\n", filesize);
    exit(1);
  }

  if(offset < 0) {
    if(memcmp(data, zeros, -offset) != 0) {
      fprintf(stderr, "READ ZERO FAIL: %d\n", errno);
      exit(1);
    }

    data -= offset;
    len  += offset;
    offset = 0;
  }

  if(memcmp(data, filedata + offset, len) != 0) {
    fprintf(stderr, "READ DATA FAIL: %d\n", errno);
    exit(1);
  }

  bytes_read += len;
  n_reads += 1;
}

void write(const char* file, const char* data, int64_t length, int64_t offset) {
  if(verbose) printf("WRITE: %8" PRId64 " bytes at %8" PRId64 "\n", length, offset);

  // Update our in-memory copy of the file:
  memcpy(filedata + offset, data, length);
  if(length > 0) {
    // Writes of length zero do not modify the file:
    filesize = max(filesize, offset + length);
  }

  // Update the real file:
  int fd = openfile(file, O_WRONLY);
  int64_t result = pwrite(fd, data, length, fileoffset + offset);

  if(result != length) {
    fprintf(stderr, "WRITE FAIL (%" PRId64 " != %" PRId64 "): %d\n", result, length, errno);
    close(fd);
    exit(1);
  }

  result = fsync(fd);
  close(fd);

  if(result != 0) {
    fprintf(stderr, "WRITE SYNC FAIL: %d\n", errno);
    exit(1);
  }

  bytes_written += length;
  n_writes += 1;
}

void trunc(const char* file, int64_t offset) {
  if(verbose) printf("TRUNC: %8" PRId64 " bytes\n", offset);
  if(offset < filesize) {
    bytes_chopped  += filesize - offset;
    n_chops += 1;
  }
  else {
    bytes_extended += offset - filesize;
    n_exts += 1;
  }

  // Update our in-memory copy of the file:
  memset(filedata + offset, 0, MAX_SIZE - offset);
  filesize = offset;

  // Update the real file:
  int fd = openfile(file, O_WRONLY);
  int result = ftruncate(fd, fileoffset + offset);
  close(fd);

  if(result != 0) {
    fprintf(stderr, "TRUNCATE FAIL: %d\n", errno);
    exit(1);
  }
}

void test_write(const char* file, int64_t length, int64_t offset) {
  assert(offset + length <= MAX_SIZE);

  char buffer[MAX_CALL*2 + 2048];
  randbytes(buffer, length);
  write(file, buffer, length, offset);

  length += 2048;
  offset -= 1024;

  if(offset + fileoffset < 0) {
    offset = -fileoffset;
  }

  read(file, buffer, length, offset);
}

void test_trunc(const char* file, int64_t offset) {
  assert(offset <= MAX_SIZE);
  trunc(file, offset);

  offset -= 1024;
  if(offset + fileoffset < 0) {
    offset = -fileoffset;
  }

  char buffer[2048];
  read(file, buffer, 2048, offset);
}

void usage(const char* message = NULL) {
  if(message) {
    fprintf(stderr, "%s\n\n", message);
  }

  fprintf(stderr, "USAGE: test-fileio [options] [test-file]\n");
  fprintf(stderr, "  -s <seed>   Run with a specific random seed.\n");
  fprintf(stderr, "  -d <depth>  \n");
  fprintf(stderr, "  -n <loops>  \n");
  exit(1);
}

int main(int argc, char** argv) {
  int seed  = time(NULL);
  int loops = 100;
  int depth = 6;
  int c;

  while((c = getopt(argc, argv, "s:d:n:v")) != -1) {
    switch(c) {
    case 's': seed  = atoi(optarg); break;
    case 'n': loops = atoi(optarg); break;
    case 'd': depth = atoi(optarg); break;
    case 'v': verbose = true;       break;
    default:
      usage();
    }
  }

  if(optind != argc - 1) {
    usage("No test file given.");
  }

  if(seed == 0) {
    usage("Failed to parse seed.");
  }

  if(depth < 1 || depth > 9) {
    usage("Depth must be between one and nine.");
  }

  if(loops < 1 ) {
    usage("Loops must be positive.");
  }

  const char* file = argv[optind];
  printf("Running tests on file %s...\n", file);
  printf(" - Seed:  %d\n", seed);
  printf(" - Loops: %d\n", loops);
  printf(" - Depth: %d\n", depth);
  srand(seed);

  int64_t offsets[] = {
    0,            // The very start of the file
    20480,        // In the middle of the direct blocks
    40960,        // At the end of the direct blocks
    1089536,      // In the middle of the singly-indirect blocks
    2138112,      // At the end of the singly-indirect blocks
    539009024,    // In the middle of the doubly-indirect blocks
    1075879936,   // At the end of the doubly-indirect blocks
    275953786880, // In the middle of the triply-indirect blocks
    550831693824  // At the end of the triply-indirect blocks
  };

  int64_t start = getusec();
  for(int i = 0; i < depth; ++i) {
    int fd = open(file, O_WRONLY | O_CREAT | O_TRUNC);
    chmod(file, 0644);
    close(fd);

    filesize = 0;
    fileoffset = offsets[i];
    memset(filedata, 0, MAX_SIZE);
    printf("Running %d loops at depth %" PRId64 "...\n", loops, fileoffset);

    for(int j = 0; j < loops; ++j) {
      test_write(file, randomize(MAX_CALL,    1), randomize(MAX_CALL,    1));
      test_write(file, randomize(1024,        1), randomize(1024,        1));
      test_write(file, randomize(MAX_CALL, 1024), randomize(MAX_CALL, 1024));
      test_write(file, randomize(4096,     1024), randomize(4096,     1024));

      test_trunc(file, randomize(MAX_CALL,    1));
      test_trunc(file, randomize(MAX_CALL, 1024));
    }
  }

  int64_t end = getusec() - start;
  printf("All tests passed in %f seconds!\n", float(end) / 1000000);
  printf("  %5" PRId64 " reads:   %12" PRId64 " bytes\n", n_reads,  bytes_read);
  printf("  %5" PRId64 " writes:  %12" PRId64 " bytes\n", n_writes, bytes_written);
  printf("  %5" PRId64 " extends: %12" PRId64 " bytes\n", n_exts,   bytes_extended);
  printf("  %5" PRId64 " chops:   %12" PRId64 " bytes\n", n_chops,  bytes_chopped);

  return 0;
}
