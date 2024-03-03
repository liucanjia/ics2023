#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#define SDL_malloc  malloc
#define SDL_free    free
#define SDL_realloc realloc

#define SDL_STBIMAGE_IMPLEMENTATION
#include "SDL_stbimage.h"

SDL_Surface* IMG_Load_RW(SDL_RWops *src, int freesrc) {
  assert(src->type == RW_TYPE_MEM);
  assert(freesrc == 0);
  return NULL;
}

SDL_Surface* IMG_Load(const char *filename) {
  int file_size = 0;
  // open the file
  FILE *fp = fopen(filename, "r");
  assert(fp);
  
  // get the file size
  fseek(fp, 0, SEEK_END);
  file_size = ftell(fp);

  // read the file in buf
  char *buf = NULL;
  buf = (char *)SDL_malloc(sizeof(char) * file_size);
  fseek(fp, 0, SEEK_SET);

  int fd = fileno(fp);
  int ret = read(fd, (void*)buf, file_size * sizeof(char));
  
  assert(ret == file_size);
  
  // use func STBIMG_LoadFromMemory() to get the picture info
  SDL_Surface *surface = STBIMG_LoadFromMemory(buf, file_size);
  
  assert(surface);
  // close the file and free the buf
  fclose(fp);
  SDL_free(buf);

  return surface;
}

int IMG_isPNG(SDL_RWops *src) {
  return 0;
}

SDL_Surface* IMG_LoadJPG_RW(SDL_RWops *src) {
  return IMG_Load_RW(src, 0);
}

char *IMG_GetError() {
  return "Navy does not support IMG_GetError()";
}
