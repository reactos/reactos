/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */
#define logf _dummy_logf
#include <math.h>
#undef logf

float logf(float _X)
{
  return ((float)log((double)_X));
}
