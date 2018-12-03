#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#define min(a, b) ((a) < (b)? (a) : (b))
#define max(a, b) ((a) > (b)? (a) : (b))

const int64_t MAX_CALL = 1024 * 1024;
const int64_t MAX_SIZE = MAX_CALL * 2 + 2048;
const char    zeros[1024] = {0};

int64_t filesize = 0;
int64_t fileoffset = 0;
char    filedata[MAX_SIZE];


void randbytes(char* buffer, int64_t size) {
  for(int i = 0; i < size; ++i) {
    buffer[i] = char(rand() % 26) + 'a';
  }

  buffer[size] = '\0';
}

void read(FILE* file, char* data, int64_t length, int64_t offset) {
  printf("READ:  %8" PRId64 " bytes at %8" PRId64 "\n", length, offset);
  int64_t len = min(filesize - offset, length);

  fseek(file, fileoffset + offset, SEEK_SET);
  int64_t result = fread(data, 1, length, file);
  // fwrite(data, 1, 32, stdout);

  if(result != len) {
    fprintf(stderr, "READ FAIL (%" PRId64 " != %" PRId64 ")\n", result, len);
    exit(1);
  }

  if(offset < 0) {
    if(memcmp(data, zeros, -offset) != 0) {
      fprintf(stderr, "READ ZERO FAIL\n");
      // fwrite(data, 1, 32, stdout);
      fclose(file);
      exit(1);
    }

    data -= offset;
    len  += offset;
    offset = 0;
  }

  if(memcmp(data, filedata + offset, len) != 0) {
    fprintf(stderr, "READ DATA FAIL\n");
    // fwrite(data, 1, 32, stdout);
    // fwrite(filedata + offset, 1, 32, stdout);
    fclose(file);
    exit(1);
  }
}

void write(FILE* file, const char* data, int64_t length, int64_t offset) {
  printf("WRITE: %8" PRId64 " bytes at %8" PRId64 "\n", length, offset);

  // Update our in-memory copy of the file:
  filesize = max(filesize, offset + length);
  memcpy(filedata + offset, data, length);
  // fwrite(data, 1, 32, stdout);

  fseek(file, fileoffset + offset, SEEK_SET);
  int64_t result = fwrite(data, 1, length, file);

  if(result != length) {
    fprintf(stderr, "WRITE FAIL (%" PRId64 " != %" PRId64 ")\n", result, length);
    fclose(file);
    exit(1);
  }
}

void trunc(FILE* file, int64_t offset) {
  (void) file;
  (void) offset;
  // TODO!
}

void test(FILE* file, int64_t max_size) {
  int64_t offset = rand() % MAX_CALL;
  int64_t length = rand() % max_size;

  if(offset + length > MAX_SIZE) {
    length = MAX_SIZE - offset;
  }

  char buffer[MAX_CALL*2 + 2048];
  randbytes(buffer, length);
  write(file, buffer, length, offset);

  length += 2048;
  offset -= 1024;

  if(offset + fileoffset < 0) {
    offset = -fileoffset;
  }

  read(file, buffer + 7, length, offset);
}

int main(int argc, char** argv) {
  if(argc != 2) {
    fprintf(stderr, "USAGE: %s [testfile]\n", argv[0]);
    exit(1);
  }

  int seed = time(NULL);
  srand(seed);

  FILE* file = fopen(argv[1], "w+");
  printf("Running with file %s and seed %d...\n", argv[1], seed);

  for(int i = 0; i < 50; ++i) {
    test(file, MAX_CALL);
    test(file, 1024);
  }

  fclose(file);
  return 0;
}
