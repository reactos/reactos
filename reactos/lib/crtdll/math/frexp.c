/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <math.h>

double
frexp(double x, int *exptr)
{
  union {
    double d;
    unsigned char c[8];
  } u;

  u.d = x;
  /*
   * The format of the number is:
   * Sign, 12 exponent bits, 51 mantissa bits
   * The exponent is 1023 biased and there is an implicit zero.
   * We get the exponent from the upper bits and set the exponent
   * to 0x3fe (1022).
   */
  *exptr = (int)(((u.c[7] & 0x7f) << 4) | (u.c[6] >> 4)) - 1022;
  u.c[7] &= 0x80;
  u.c[7] |= 0x3f;
  u.c[6] &= 0x0f;
  u.c[6] |= 0xe0;
  return u.d;
}
