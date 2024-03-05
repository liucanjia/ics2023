#include <am.h>
#include <navy.h>

#define DISPINFOPATH "/proc/dispinfo"
#define FBPATH "/dev/fb"

void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
  int fd = open(DISPINFOPATH, 0, 0);
  
  char buf[64];
  int ret = read(fd, buf, sizeof(buf));
  close(fd);

  int w, h;
  sscanf(buf, "WIDTH:%d\nHEIGHT:%d", &w, &h);

  *cfg = (AM_GPU_CONFIG_T) {
    .present = true, .has_accel = false,
    .width = w, .height = h,
    .vmemsz = 0
  };
}

void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
  int x = ctl->x, y = ctl->y, w = ctl->w, h = ctl->h;
  int fd = open(FBPATH, 0, 0);

  int screen_w = 0;
  AM_GPU_CONFIG_T cfg;
  __am_gpu_config(&cfg);
  screen_w = cfg.width;

  uint32_t *pixels = (uint32_t *)ctl->pixels;
  lseek(fd, y * screen_w + x, SEEK_SET);
  for (int i = 0; i < h; i++) {
    write(fd, (const void *)(pixels + w * i), w * sizeof(uint32_t));
    lseek(fd, screen_w, SEEK_CUR);
  }
  write(fd, NULL, 0);

  close(fd);
}

void __am_gpu_status(AM_GPU_STATUS_T *status) {
  status->ready = true;
}