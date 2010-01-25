/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */
#include <fenv.h>
#include <math.h>
#include <errno.h>
#define FE_ROUNDING_MASK \
  (FE_TONEAREST | FE_DOWNWARD | FE_UPWARD | FE_TOWARDZERO)

float
modff (float value, float* iptr)
{
  float int_part = 0.0F;
  unsigned short saved_cw;
  unsigned short tmp_cw;
  /* truncate */ 
  asm ("fnstcw %0;" : "=m" (saved_cw)); /* save control word */
  tmp_cw = (saved_cw & ~FE_ROUNDING_MASK) | FE_TOWARDZERO;
  asm ("fldcw %0;" : : "m" (tmp_cw));
  asm ("frndint;" : "=t" (int_part) : "0" (value)); /* round */
  asm ("fldcw %0;" : : "m" (saved_cw)); /* restore saved cw */
  if (iptr)
    *iptr = int_part;
  return (isinf (value) ?  0.0F : value - int_part);
}
