#include <common.h>
#include "proc.h"
#include "syscall.h"
#include "am.h"
#include <fs.h>
#include <sys/time.h>

#define APP_PATH "/bin/nterm"

extern void naive_uload(PCB *pcb, const char *filename);

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;
  a[1] = c->GPR2;
  a[2] = c->GPR3;
  a[3] = c->GPR4;

  uintptr_t return_val = 0;

  switch (a[0]) {
    case SYS_exit:
#ifdef CONFIG_STRACE
      printf("Syscall exit, status: %d\n", a[1]);
#endif
      naive_uload(NULL, APP_PATH);
      break;
    case SYS_yield:
#ifdef CONFIG_STRACE
      printf("Syscall yield.\n");
#endif
      yield();
      return_val = 0;
      break;
    case SYS_open:
      return_val = fs_open((char*)a[1], a[2], a[3]);
#ifdef CONFIG_STRACE
      printf("Syscall open, filename: %s, flags: %d, mode: %d, return_val(fd): %d.\n", (char*)a[1], a[2], a[3], return_val);
#endif
      break;
    case SYS_read:
      return_val = fs_read(a[1], (void*)a[2], a[3]);
#ifdef CONFIG_STRACE
      printf("Syscall read, filename: %s(fd: %d), buf: %p, len: %d, return_val: %d.\n", fs_filename(a[1]), a[1], a[2], a[3], return_val);
#endif
      break;
    case SYS_write:
        return_val = fs_write(a[1], (const void *)a[2], a[3]);
#ifdef CONFIG_STRACE
      switch (a[1]) {
        case FD_STDIN:
          printf("Syscall write to FD_STDIN, buf: %p, count: %d, return_val: %d\n", a[2], a[3], return_val);
          break;
        case FD_STDOUT:
          printf("Syscall write to FD_STDOUT, buf: %p, count: %d, return_val: %d\n", a[2], a[3], return_val);
          break;
        case FD_STDERR:
          printf("Syscall write to FD_STDERR, buf: %p, count: %d, return_val: %d\n", a[2], a[3], return_val);
          break;
        default:
          printf("Syscall write to file: %s(fd: %d), buf: %p, count: %d, return_val: %d\n", fs_filename(a[1]), a[1], a[2], a[3], return_val);
          break;
      }
#endif
      break;
    case SYS_close:
      return_val = fs_close(a[1]);
#ifdef CONFIG_STRACE
      printf("Syscall close, filename: %s, fd: %d, return_val: %d.\n", fs_filename(a[1]), a[1], return_val);
#endif
      break;
    case SYS_lseek:
      return_val = fs_lseek(a[1], a[2], a[3]);
#ifdef CONFIG_STRACE
      printf("Syscall lseek, filename: %s(fd: %d), offset: %d, whence: %d.\n", fs_filename(a[1]), a[1], a[2], a[3]);
#endif
      break;
    case SYS_brk:
      return_val = 0;
#ifdef CONFIG_STRACE
      printf("Syscall brk, program break: %p, return_val: %d.\n", a[1], return_val);
#endif
      break;
    case SYS_execve:
#ifdef CONFIG_STRACE
      printf("Syscall execve, filename: %s.\n", (char*)a[1]);
#endif
      naive_uload(NULL, (char*)a[1]);
      break;
    case SYS_gettimeofday:
      struct timeval* tv = (struct timeval*)a[1];
      uint64_t us = io_read(AM_TIMER_UPTIME).us;
      if (tv != NULL) {
        tv->tv_sec = us / 1000000;
        tv->tv_usec = us % 1000000;
      }
      return_val = 0;
      break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
  c->GPRx = return_val;
}
