/* Math functions for i387.
   Copyright (C) 1995, 1996, 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by John C. Bowman <bowman@ipp-garching.mpg.de>, 1995.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include <windows.h>
#include <stdlib.h>

double atan (double __x);
double atan2 (double __y, double __x);
double ceil (double __x);
double cos (double __x);
double fabs (double __x);
double floor (double __x);
long _ftol (double fl);
double log (double __x);
double __log2 (double __x);
double pow (double __x, double __y);
double sin (double __x);
double sqrt (double __x);
double tan (double __x);
div_t div(int num, int denom);
int mod(int num, int denom);

double atan (double __x)
{
  register double __value;
  __asm __volatile__
    ("fld1\n\t"
     "fpatan"
     : "=t" (__value) : "0" (__x));

  return __value;
}

double atan2 (double __y, double __x)
{
  register double __value;
  __asm __volatile__
    ("fpatan\n\t"
     "fld %%st(0)"
     : "=t" (__value) : "0" (__x), "u" (__y));

  return __value;
}

double ceil (double __x)
{
  register double __value;
  __volatile unsigned short int __cw, __cwtmp;

  __asm __volatile ("fnstcw %0" : "=m" (__cw));
  __cwtmp = (__cw & 0xf3ff) | 0x0800; /* rounding up */
  __asm __volatile ("fldcw %0" : : "m" (__cwtmp));
  __asm __volatile ("frndint" : "=t" (__value) : "0" (__x));
  __asm __volatile ("fldcw %0" : : "m" (__cw));

  return __value;
}

double cos (double __x)
{
  register double __value;
  __asm __volatile__
    ("fcos"
     : "=t" (__value): "0" (__x));

  return __value;
}

double fabs (double __x)
{
  register double __value;
  __asm __volatile__
    ("fabs"
     : "=t" (__value) : "0" (__x));

  return __value;
}

double floor (double __x)
{
  register double __value;
  __volatile unsigned short int __cw, __cwtmp;

  __asm __volatile ("fnstcw %0" : "=m" (__cw));
  __cwtmp = (__cw & 0xf3ff) | 0x0400; /* rounding down */
  __asm __volatile ("fldcw %0" : : "m" (__cwtmp));
  __asm __volatile ("frndint" : "=t" (__value) : "0" (__x));
  __asm __volatile ("fldcw %0" : : "m" (__cw));

  return __value;
}

long _ftol (double fl)
{
  return (long)fl;
}

double log (double __x)
{
  register double __value;
  __asm __volatile__
    ("fldln2\n\t"
     "fxch\n\t"
     "fyl2x"
     : "=t" (__value) : "0" (__x));

  return __value;
}

double __log2 (double __x)
{
  register double __value;
  __asm __volatile__
    ("fld1\n\t"
     "fxch\n\t"
     "fyl2x"
     : "=t" (__value) : "0" (__x));

  return __value;
}

double pow (double __x, double __y)
{
  register double __value, __exponent;
  long __p = (long) __y;

  if (__x == 0.0 && __y > 0.0)
    return 0.0;
  if (__y == (double) __p)
    {
      double __r = 1.0;
      if (__p == 0)
        return 1.0;
      if (__p < 0)
        {
          __p = -__p;
          __x = 1.0 / __x;
        }
      while (1)
        {
          if (__p & 1)
            __r *= __x;
          __p >>= 1;
          if (__p == 0)
            return __r;
          __x *= __x;
        }
      /* NOTREACHED */
    }
  __asm __volatile__
    ("fmul      %%st(1)         # y * log2(x)\n\t"
     "fst       %%st(1)\n\t"
     "frndint                   # int(y * log2(x))\n\t"
     "fxch\n\t"
     "fsub      %%st(1)         # fract(y * log2(x))\n\t"
     "f2xm1                     # 2^(fract(y * log2(x))) - 1\n\t"
     : "=t" (__value), "=u" (__exponent) :  "0" (__log2 (__x)), "1" (__y));
  __value += 1.0;
  __asm __volatile__
    ("fscale"
     : "=t" (__value) : "0" (__value), "u" (__exponent));

  return __value;
}

double sin (double __x)
{
  register double __value;
  __asm __volatile__
    ("fsin"
     : "=t" (__value) : "0" (__x));

  return __value;
}

double sqrt (double __x)
{
  register double __value;
  __asm __volatile__
    ("fsqrt"
     : "=t" (__value) : "0" (__x));

  return __value;
}

double tan (double __x)
{
  register double __value;
  register double __value2 __attribute__ ((unused));
  __asm __volatile__
    ("fptan"
     : "=t" (__value2), "=u" (__value) : "0" (__x));

  return __value;
}

div_t div(int num, int denom)
{
  div_t r;
  if (num > 0 && denom < 0) {
    num = -num;
    denom = -denom;
  }
  r.quot = num / denom;
  r.rem = num % denom;
  if (num < 0 && denom > 0)
  {
    if (r.rem > 0)
    {
      r.quot++;
      r.rem -= denom;
    }
  }
  return r;
}

int mod(int num, int denom)
{
  div_t dvt = div(num, denom);
  return dvt.rem;
}

//FIXME! Is there a better algorithm. like FT_MulDiv
INT STDCALL EngMulDiv(
	     INT nMultiplicand,
	     INT nMultiplier,
	     INT nDivisor)
{
#if SIZEOF_LONG_LONG >= 8
    long long ret;

    if (!nDivisor) return -1;

    /* We want to deal with a positive divisor to simplify the logic. */
    if (nDivisor < 0)
    {
      nMultiplicand = - nMultiplicand;
      nDivisor = -nDivisor;
    }

    /* If the result is positive, we "add" to round. else, we subtract to round. */
    if ( ( (nMultiplicand <  0) && (nMultiplier <  0) ) ||
	 ( (nMultiplicand >= 0) && (nMultiplier >= 0) ) )
      ret = (((long long)nMultiplicand * nMultiplier) + (nDivisor/2)) / nDivisor;
    else
      ret = (((long long)nMultiplicand * nMultiplier) - (nDivisor/2)) / nDivisor;

    if ((ret > 2147483647) || (ret < -2147483647)) return -1;
    return ret;
#else
    if (!nDivisor) return -1;

    /* We want to deal with a positive divisor to simplify the logic. */
    if (nDivisor < 0)
    {
      nMultiplicand = - nMultiplicand;
      nDivisor = -nDivisor;
    }

    /* If the result is positive, we "add" to round. else, we subtract to round. */
    if ( ( (nMultiplicand <  0) && (nMultiplier <  0) ) ||
	 ( (nMultiplicand >= 0) && (nMultiplier >= 0) ) )
      return ((nMultiplicand * nMultiplier) + (nDivisor/2)) / nDivisor;

    return ((nMultiplicand * nMultiplier) - (nDivisor/2)) / nDivisor;

#endif
}
