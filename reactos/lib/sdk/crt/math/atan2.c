/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */
#include <math.h>

double
atan2 (double y, double x)
{
  long double res = 0.0L;
#ifdef __GNUC__
  asm ("fpatan" : "=t" (res) : "u" (y), "0" (x) : "st(1)");
#endif
  return res;
}
