/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */
#include <math.h>

_Check_return_
float
__cdecl
sqrtf(float x)
{
   return (float)sqrt((double)x);
}
