#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>
#include <stdbool.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

static void reverse(char *str, int len) {
  char *end = str + len - 1;

  char tmp;
  while (str++ < end--) {
    tmp = *str;
    *str = *end;
    *end = tmp;
  }
}

static int itoa(int val, char *str) {
  if (val == 0x80000000) {
    memcpy(str, "-2147483648", 11);
    return 11;
  }

  int len = 0, bit;
  bool sign = false;

  if (val < 0) {
    sign = true;
    val = -val;
  }

  do {
    bit = val % 10;
    str[len++] = '0' + bit;
    val /= 10;
  }while (val > 0);

  if (sign == true) {
    str[len++] = '-';
  }

  reverse(str, len);
  return len;
}

int printf(const char *fmt, ...) {
  int val = 0;
  char buf[256];

  va_list ap;
  va_start(ap, fmt);
  val = vsprintf(buf, fmt, ap);
  va_end(ap);

  putstr(buf);
  return val;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  int val;
  char *s, *ptr = out;

  for (; *fmt != '\0'; fmt++) {
    if (*fmt == '%'){
      fmt++;
      switch (*fmt) {
        case 'd':
          val = va_arg(ap, int);
          out += itoa(val, out);
          break;
        case 's':
          s = va_arg(ap, char*);
          while (*s != '\0') {
            *out++ = *s++;
          }
          break;
      }
    } else {
      *out++ = *fmt;
    }
  }

  *out = '\0';
  return out - ptr;
}

int sprintf(char *out, const char *fmt, ...) {
  int val = 0;
  
  va_list ap;
  va_start(ap, fmt);
  val = vsprintf(out, fmt, ap);
  va_end(ap);

  return val;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
