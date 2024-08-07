#include <common.h>

#if defined(MULTIPROGRAM) && !defined(TIME_SHARING)
# define MULTIPROGRAM_YIELD() yield()
#else
# define MULTIPROGRAM_YIELD()
#endif

#define NAME(key) \
  [AM_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [AM_KEY_NONE] = "NONE",
  AM_KEYS(NAME)
};

size_t serial_write(const void *buf, size_t offset, size_t len) {
  for (size_t i = 0; i < len; i++) {
    putch(*(char*)(buf + i));
  }
  return len;
}

size_t events_read(void *buf, size_t offset, size_t len) {
  AM_INPUT_KEYBRD_T ev = io_read(AM_INPUT_KEYBRD);
  if (ev.keycode != AM_KEY_NONE) {
    if (ev.keydown) {
      return snprintf(buf, len, "keydown %s (keycode: %d)\n", keyname[ev.keycode], ev.keycode);
    } else {
      return snprintf(buf, len, "keyup %s (keycode: %d)\n", keyname[ev.keycode], ev.keycode);
    }
  }
  return 0;
}

size_t dispinfo_read(void *buf, size_t offset, size_t len) {
  int w = io_read(AM_GPU_CONFIG).width, h = io_read(AM_GPU_CONFIG).height;
  return snprintf(buf, len, "%s:%d\n%s:%d\n", "WIDTH", w, "HEIGHT", h);
}

size_t fb_write(const void *buf, size_t offset, size_t len) {
  if (buf == NULL && len == 0) {
    io_write(AM_GPU_FBDRAW, 0, 0, NULL, 0, 0, true);
    return 0;
  }

  int w = io_read(AM_GPU_CONFIG).width;
  int x = offset % w, y = offset / w;
  // draw one row at a time
  
  io_write(AM_GPU_FBDRAW, x, y, (uint32_t*)buf, len / sizeof(uint32_t), 1, false);
  return 0;
}

void init_device() {
  Log("Initializing devices...");
  ioe_init();
}
