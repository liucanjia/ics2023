#include <fs.h>

extern size_t ramdisk_read(void *buf, size_t offset, size_t len);
extern size_t ramdisk_write(const void *buf, size_t offset, size_t len);

typedef size_t (*ReadFn) (void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn) (const void *buf, size_t offset, size_t len);

typedef struct {
  char *name;
  size_t size;
  size_t disk_offset;
  size_t open_offset;
  ReadFn read;
  WriteFn write;
} Finfo;

size_t invalid_read(void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

size_t invalid_write(const void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  [FD_STDIN]  = {"stdin", 0, 0, 0, invalid_read, invalid_write},
  [FD_STDOUT] = {"stdout", 0, 0, 0, invalid_read, invalid_write},
  [FD_STDERR] = {"stderr", 0, 0, 0, invalid_read, invalid_write},
#include "files.h"
};

void init_fs() {
  // TODO: initialize the size of /dev/fb
}

int fs_open(const char *pathname, int flags, int mode) {
  int file_count = sizeof(file_table)/sizeof(file_table[0]);
  for (int i = 0; i < file_count; i++) {
    if (strcmp(pathname, file_table[i].name) == 0) {
      return i;
    }
  }

  panic("filename is invaild.");
}

size_t fs_read(int fd, void *buf, size_t len) {
  int file_count = sizeof(file_table)/sizeof(file_table[0]);

  assert(fd >= 0 && fd < file_count);
  
  len = file_table[fd].open_offset + len > file_table[fd].size? file_table[fd].size - file_table[fd].open_offset: len;
  int ret = ramdisk_read(buf, file_table[fd].disk_offset + file_table[fd].open_offset, len);
  assert(ret == len);

  file_table[fd].open_offset += len;
  return len;
}

size_t fs_write(int fd, const void *buf, size_t len) {
  int file_count = sizeof(file_table)/sizeof(file_table[0]);

  assert(fd >= 0 && fd < file_count);

  len = file_table[fd].open_offset + len > file_table[fd].size? file_table[fd].size - file_table[fd].open_offset: len;
  int ret = ramdisk_write(buf, file_table[fd].disk_offset + file_table[fd].open_offset, len);
  assert(ret == len);

  file_table[fd].open_offset += len;
  return len;
}

size_t fs_lseek(int fd, size_t offset, int whence) {
  switch (whence) {
    case SEEK_SET:
      file_table[fd].open_offset = 0;
      break;
    case SEEK_CUR:
      break;
    case SEEK_END:
      file_table[fd].open_offset = file_table[fd].size;
      break;
    default:
      panic("whence value is invaild");
  }

  if (file_table[fd].open_offset + offset > file_table[fd].size) {
    return -1;
  } else {
    file_table[fd].open_offset += offset;
  }

  return file_table[fd].open_offset;
}

int fs_close(int fd) {
  file_table[fd].open_offset = 0;
  return 0;   // close always success because doesn't maintain the file open state
}

char* fs_filename(int fd) {
  return file_table[fd].name;
}