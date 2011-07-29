/* Math functions for i387.
   Copyright (C) 1995, 1996, 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by John C. Bowman <bowman@ipp-garching.mpg.de>, 1995.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <math.h>

double exp (double __x);

double exp (double __x)
{
#ifdef __GNUC__
  register double __value, __exponent;
  __asm __volatile__
    ("fldl2e                    # e^x = 2^(x * log2(e))\n\t"
     "fmul      %%st(1)         # x * log2(e)\n\t"
     "fst       %%st(1)\n\t"
     "frndint                   # int(x * log2(e))\n\t"
     "fxch\n\t"
     "fsub      %%st(1)         # fract(x * log2(e))\n\t"
     "f2xm1                     # 2^(fract(x * log2(e))) - 1\n\t"
     : "=t" (__value), "=u" (__exponent) : "0" (__x));
  __value += 1.0;
  __asm __volatile__
    ("fscale"
     : "=t" (__value) : "0" (__value), "u" (__exponent));

  return __value;
#else
  register double __val;
  __asm
  {
    fld1                        // store 1.0 for later use
    fld __x
    fldl2e                      // e^x = 2^(x * log2(e))
    fmul    st,st(1)            // x * log2(e)
    fld     st(0)
    frndint                     // int(x * log2(e))
    fsub    st,st(1)            // fract(x * log2(e))
    fxch
    f2xm1                       // 2^(fract(x * log2(e))) - 1
    fadd    st,st(3)            // + 1.0
    fscale
    fstp __val
  }
  return __val;
#endif /*__GNUC__*/
}
