/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */
#include <math.h>
#include <errno.h>

double
sqrt (double x)
{
  if (x < 0.0L )
    {
#ifndef _LIBCNT_
      errno = EDOM;
#endif
      return NAN;
    }
  else
    {
      long double res = 0.0L;
      asm ("fsqrt" : "=t" (res) : "0" (x));
      return res;
    }
}
