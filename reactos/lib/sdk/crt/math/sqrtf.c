/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */
#include <math.h>
#include <errno.h>

float
sqrtf (float x)
{
  if (x < 0.0F )
    {
      errno = EDOM;
      return NAN;
    }
  else
    {
      float res = 0.0F;
      asm ("fsqrt" : "=t" (res) : "0" (x));
      return res;
    }
}
