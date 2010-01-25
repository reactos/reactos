/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */
#include <math.h>

double
fabs (double x)
{
  double res = 0.0;
#ifdef __GNUC__
  asm ("fabs;" : "=t" (res) : "0" (x));
#endif
  return res;
}
