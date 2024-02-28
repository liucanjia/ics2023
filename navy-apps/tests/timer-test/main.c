#include <assert.h>

int main() {
  uint32_t ms = 500;
  while (1) {
    while (NDL_GetTicks() < ms);
    ms += 500;
    printf("hello world!!!!\n");
  }

  return 0;
}
