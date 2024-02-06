#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

static void reverse(char *begin, char *end) {

  char tmp;
  while (begin < end) {
    tmp = *begin;
    *begin++ = *end;
    *end-- = tmp;
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

  reverse(str, str + len - 1);
  return len;
}

int printf(const char *fmt, ...) {
  int val = 0;
  char buf[2048];

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
  int zero_flag = 0;

  for (; *fmt != '\0'; fmt++) {
    if (*fmt == '%'){
      int val_width = 0;

      fmt++;
      // %0md
      if (*fmt == '0') {
        zero_flag = 1;
        fmt++;
      }
      while(*fmt >= '0' && *fmt <= '9') {
        val_width = val_width * 10 + (*fmt - '0');
        fmt++;
      }

      switch (*fmt) {
        case 'd': {
          int val_len = 0;
          val = va_arg(ap, int);
          val_len = itoa(val, out);
          if (val_len < val_width) {
            for (int i = 0; i < val_width - val_len; i++) {
              *(out + val_len + i) = zero_flag ? '0': ' ';
            }
            reverse(out, out + val_len - 1);
            reverse(out, out + val_width - 1);
          }
          out += val_width > val_len ? val_width: val_len;
          break;
        }
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
