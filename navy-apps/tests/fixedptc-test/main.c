#include <fixedptc.h>
#include <stdio.h>

#define A_VAL (1.2)
#define B_VAL (-1.2)
#define A_STR "1.2"
#define B_STR "-1.2"

int main() {
  fixedpt a = fixedpt_rconst(A_VAL);
  fixedpt b = fixedpt_rconst(B_VAL);
  int c = 3;
  printf("a = %s, fixedpt a = %d\n", A_STR, a);
  printf("b = %s, fixedpt b = %d\n", B_STR, b);
  printf("a = %s, floor(a) = %d\n", A_STR, fixedpt_toint(fixedpt_floor(a)));
  printf("a = %s, ceil(a) = %d\n", A_STR, fixedpt_toint(fixedpt_ceil(a)));
  printf("b = %s, floor(b) = %d\n", B_STR, fixedpt_toint(fixedpt_floor(b)));
  printf("b = %s, ceil(b) = %d\n", B_STR, fixedpt_toint(fixedpt_ceil(b)));
  printf("a = %s, abs(a) = %d\n", A_STR, fixedpt_toint(fixedpt_abs(a)));
  printf("b = %s, abs(b) = %d\n", B_STR, fixedpt_toint(fixedpt_abs(b)));

  printf("c = %d, fixedpt c = %d\n", c, fixedpt_rconst(c));
  printf("a * c = %d(fixedpt %d), a / c = %d(fixedpt %d)\n", fixedpt_toint(fixedpt_muli(a, c)), fixedpt_muli(a, c), fixedpt_toint(fixedpt_divi(a, c)), fixedpt_divi(a, c));
  printf("b * c = %d(fixedpt %d), b / c = %d(fixedpt %d)\n", fixedpt_toint(fixedpt_muli(b, c)), fixedpt_muli(b, c), fixedpt_toint(fixedpt_divi(b, c)), fixedpt_divi(b, c));

  printf("PASS!!!\n");
  return 0;
}
