#include <am.h>
#include <navy.h>

#define EVENTPATH "/dev/events"

void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
  char buf[64], type[8], keyname[8];
  int keycode;

  int fd = open(EVENTPATH, 0, 0);

  if (read(fd, buf, 64)) {
    sscanf(buf, "%s %s (keycode: %d)\n", type, keyname, &keycode);
    if (strncmp(type, "keydown", sizeof("keydown")) == 0) {
      kbd->keydown = true;
    } else if (strncmp(type, "keyup", sizeof("keyup")) == 0) {
      kbd->keydown = false;
    }
    kbd->keycode = keycode;

    close(fd);
  } else {
    kbd->keycode = 0;
  }

}