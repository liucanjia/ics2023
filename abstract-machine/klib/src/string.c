#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  if (s == NULL) {
    panic("s is null pointer!");
  }

  size_t len = 0;
  while (*s != '\0') {
    len++;
    s++;
  }
  return len;
}

char *strcpy(char *dst, const char *src) {
  if (dst == NULL || src == NULL) {
    panic("dst or src is null pointer!");
  }

  char *ptr = dst;
  while ((*dst = *src) != '\0') {
    dst++;
    src++;
  }

  return ptr;
}

char *strncpy(char *dst, const char *src, size_t n) {
  if (dst == NULL || src == NULL) {
    panic("dst or src is null pointer!");
  }

  char *ptr = dst;
  while (n--) {
    if (*src != '\0') {
      *dst = *src;
      src++;
    } else {
      *dst = '\0';
    }
    dst++;
  }
  return ptr;
}

char *strcat(char *dst, const char *src) {
  if (dst == NULL || src == NULL) {
    panic("dst or src is null pointer!");
  }

  char *ptr = dst;
  while (*dst != '\0') {
    dst++;
  }

  while ((*dst = *src) != '\0') {
    dst++;
    src++;
  }

  return ptr;
}

int strcmp(const char *s1, const char *s2) {
  if (s1 == NULL || s2 == NULL) {
    panic("s1 or s2 is null pointer!");
  }

  while (1) {
    if (*s1 != *s2) {
      return *(unsigned char*)s1 - *(unsigned char*)s2;
    }
    
    if (*s1 == '\0') {
      return 0;  
    }

    s1++;
    s2++;
  }

  return 0;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  if (s1 == NULL || s2 == NULL) {
    panic("s1 or s2 is null pointer!");
  }

  while (n--) {
    if (*s1 != *s2) {
      return *(unsigned char*)s1 - *(unsigned char*)s2;
    }

    if (*s1 == '\0') {
      return 0;
    }

    s1++;
    s2++;
  }

  return 0;
}

void *memset(void *s, int c, size_t n) {
  if (s == NULL) {
    panic("s is null pointer!");
  }

  unsigned char *ptr = s;
  while (n--) {
    *ptr = c;
    ptr++;
  }

  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  if (dst == NULL || src == NULL) {
    panic("dst or src is null pointer!");
  }

  unsigned char *ptr1 = dst;
  const unsigned char *ptr2 = src;

  if (ptr1 < ptr2) {
    while (n--) {
      *ptr1 = *ptr2;
      ptr1++;
      ptr2++;
    }
  } else if (ptr1 > ptr2) {
    ptr1 += n - 1;
    ptr2 += n - 1;
    while(n--) {
      *ptr1 = *ptr2;
      ptr1--;
      ptr2--;
    }
  }

  return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
  if (out == NULL || in == NULL) {
    panic("out or in is null pointer!");
  }

  unsigned char *ptr1 = out;
  const unsigned char *ptr2 = in;
  while (n--) {
    *ptr1 = *ptr2;
    ptr1++;
    ptr2++;
  }

  return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  if (s1 == NULL || s2 == NULL) {
    panic("s1 or s2 is null pointer!");
  }

  const unsigned char *ptr1 = s1, *ptr2 = s2;

  while (n--) {
    if (*ptr1 != *ptr2) {
      return *ptr1 - *ptr2;
    }
    ptr1++;
    ptr2++;
  }

  return 0;
}

#endif
