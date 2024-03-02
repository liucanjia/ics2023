#include <NDL.h>
#include <SDL.h>
#include <string.h>

#define keyname(k) #k,

static const char *keyname[] = {
  "NONE",
  _KEYS(keyname)
};

int SDL_PushEvent(SDL_Event *ev) {
  return 0;
}

int SDL_PollEvent(SDL_Event *ev) {
  return 0;
}

int SDL_WaitEvent(SDL_Event *event) {
  char buf[64], type[8], keyname[8];
  int keycode;
  while (1) {
    if (NDL_PollEvent(buf, sizeof(buf)) == 0) {
      continue;
    }
    sscanf(buf, "%s %s (keycode: %d)\n", type, keyname, &keycode);

    if (strncmp(type, "keydown", sizeof("keydown")) == 0) {
      event->type = event->key.type = SDL_KEYDOWN;
    } else if (strncmp(type, "keydown", sizeof("keyup")) == 0) {
      event->type = event->key.type = SDL_KEYUP;
    }
    event->key.keysym.sym = (uint8_t)keycode;
    return 1;
  }
  return 0;
}

int SDL_PeepEvents(SDL_Event *ev, int numevents, int action, uint32_t mask) {
  return 0;
}

uint8_t* SDL_GetKeyState(int *numkeys) {
  return NULL;
}
