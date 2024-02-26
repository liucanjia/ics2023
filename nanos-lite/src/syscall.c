#include <common.h>
#include "syscall.h"
#include "am.h"

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;
  a[1] = c->GPR2;
  a[2] = c->GPR3;
  a[3] = c->GPR4;

  uintptr_t return_val = 0;

  switch (a[0]) {
    case SYS_exit:
      printf("Syscall exit, arg: %d\n", a[0]);
      halt(a[1]);
      break;
    case SYS_yield:
      yield();
      return_val = 0;
      break;
    case SYS_write:
      if (a[1] == 1 || a[1] == 2) {
        for (uintptr_t i = 0; i < a[3]; i++) {
          putch(*(char*)(a[2] + i));
        }
      }
      return_val = a[3];
      break;
    case SYS_brk:
      return_val = 0;
      break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }

#ifdef CONFIG_STRACE
  printf("Syscall No: %d, arg1: %d, arg2: %d, arg3: %d, arg4: %d, arg5: %d, arg6: %d, arg7: %d, return value: %d.\n", c->GPR1, c->GPR2, c->GPR3, c->GPR4, c->GPR5, c->GPR6, c->GPR7, c->GPR8, c->GPRx);
#endif
  c->GPRx = return_val;
}
