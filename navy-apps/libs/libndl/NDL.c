#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

static int evtdev = -1;
static int fbdev = -1;
static int screen_w = 0, screen_h = 0;
static int canvas_w = 0, canvas_h = 0;
static struct timeval boot_time = {};

static void get_val(char *buf, int *val) {
  *val = 0;
  while (*buf >= '0' && *buf <= '9') {
    *val = *val * 10 + *buf - '0';
    buf++;
  }
}

uint32_t NDL_GetTicks() {
  struct timeval tv;
  gettimeofday(&tv, NULL);

  return (tv.tv_sec - boot_time.tv_sec) * 1000 + (tv.tv_usec - boot_time.tv_usec) / 1000;
}

int NDL_PollEvent(char *buf, int len) {
  int fd = open("/dev/events", 0, 0);
  int ret = read(fd, buf, len);
  close(fd);
  return ret;
}

void NDL_OpenCanvas(int *w, int *h) {
  if (getenv("NWM_APP")) {
    int fbctl = 4;
    fbdev = 5;
    screen_w = *w; screen_h = *h;
    char buf[64];
    int len = sprintf(buf, "%d %d", screen_w, screen_h);
    // let NWM resize the window and create the frame buffer
    write(fbctl, buf, len);
    while (1) {
      // 3 = evtdev
      int nread = read(3, buf, sizeof(buf) - 1);
      if (nread <= 0) continue;
      buf[nread] = '\0';
      if (strcmp(buf, "mmap ok") == 0) break;
    }
    close(fbctl);
  }
  
  int fd = open("/proc/dispinfo", 0, 0);
  char wh_buff[64];
  int ret = read(fd, wh_buff, sizeof(wh_buff));
  close(fd);

  sscanf(wh_buff, "WIDTH:%d\nHEIGHT:%d", &screen_w, &screen_h);
  
  if (*w == 0 || *h == 0) {
    *w = canvas_w = screen_w;
    *h = canvas_h = screen_h;
  } else {
    // The canvas size cannot exceed the screen size.
    *w = canvas_w = *w > screen_w ? screen_w: *w;
    *h = canvas_h = *h > screen_h ? screen_h: *h;
  }
}

void NDL_DrawRect(uint32_t *pixels, int x, int y, int w, int h) {
  // rect size cannot exceed the canvas size.
  w = w > canvas_w - x ? canvas_w - x : w;
  h = h > canvas_h - y ? canvas_h - y : h;

  FILE* fp = fopen("/dev/fb", "rw");
  int fd = fileno(fp);

  //center the canvas on the screen
  fseek(fp, ((screen_w - canvas_w) / 2 + (screen_h - canvas_h) / 2 * screen_w), SEEK_SET);
  for (int i = 0; i < h; i++) {
    write(fd, (const void*)(pixels + w * i), w * sizeof(uint32_t));
    fseek(fp, screen_w, SEEK_CUR);
  }
  write(fd, NULL, 0);
}

void NDL_OpenAudio(int freq, int channels, int samples) {
}

void NDL_CloseAudio() {
}

int NDL_PlayAudio(void *buf, int len) {
  return 0;
}

int NDL_QueryAudio() {
  return 0;
}

int NDL_Init(uint32_t flags) {
  if (getenv("NWM_APP")) {
    evtdev = 3;
  }

  gettimeofday(&boot_time, NULL);
  return 0;
}

void NDL_Quit() {
}
