#include <am.h>
#include <navy.h>

Area heap;

void putch(char ch) {
  putchar(ch);
}

void halt(int code) {
  _exit(code);

  // should not reach here
  while(1);
}
