
/*******************************************************************************
MIT License
-----------

Copyright (c) 2002-2019 Advanced Micro Devices, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this Software and associated documentaon files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*******************************************************************************/

#include "libm.h"
#include "libm_util.h"

#define FAST_BUT_GREATER_THAN_ONE_ULP  /* Helps speed by trading off a little
                                          accuracy */
#define USE_SCALEDOUBLE_1
#define USE_INFINITY_WITH_FLAGS
#define USE_HANDLE_ERROR
#include "libm_inlines.h"
#undef USE_SCALEDOUBLE_1
#undef USE_INFINITY_WITH_FLAGS
#undef USE_HANDLE_ERROR

#include "libm_errno.h"

#if (_MSC_VER >= 1920) // VS 2019+ / Compiler version 14.20
#pragma function(_hypot)
#endif

double FN_PROTOTYPE(_hypot)(double x, double y)
{
  /* Returns sqrt(x*x + y*y) with no overflow or underflow unless
     the result warrants it */

  const double large = 1.79769313486231570815e+308; /* 0x7fefffffffffffff */

#ifdef FAST_BUT_GREATER_THAN_ONE_ULP
  double r, retval;
  unsigned long long xexp, yexp, ux, uy;
#else  
  double u, r, retval, hx, tx, x2, hy, ty, y2, hs, ts;
  unsigned long long xexp, yexp, ux, uy, ut;
#endif
  int dexp, expadjust;

  GET_BITS_DP64(x, ux);
  ux &= ~SIGNBIT_DP64;
  GET_BITS_DP64(y, uy);
  uy &= ~SIGNBIT_DP64;
  xexp = (ux >> EXPSHIFTBITS_DP64);
  yexp = (uy >> EXPSHIFTBITS_DP64);

  if (xexp == BIASEDEMAX_DP64 + 1 || yexp == BIASEDEMAX_DP64 + 1)
    {
      /* One or both of the arguments are NaN or infinity. The
         result will also be NaN or infinity. */
      retval = x*x + y*y;
      if (((xexp == BIASEDEMAX_DP64 + 1) && !(ux & MANTBITS_DP64)) ||
          ((yexp == BIASEDEMAX_DP64 + 1) && !(uy & MANTBITS_DP64)))
        /* x or y is infinity. ISO C99 defines that we must
           return +infinity, even if the other argument is NaN.
           Note that the computation of x*x + y*y above will already
           have raised invalid if either x or y is a signalling NaN. */
        return infinity_with_flags(0);
      else
        /* One or both of x or y is NaN, and neither is infinity.
           Raise invalid if it's a signalling NaN */
        return retval;
    }

  /* Set x = abs(x) and y = abs(y) */
  PUT_BITS_DP64(ux, x);
  PUT_BITS_DP64(uy, y);

  /* The difference in exponents between x and y */
  dexp = (int)(xexp - yexp);
  expadjust = 0;

  if (ux == 0)
    /* x is zero */
    return y;
  else if (uy == 0)
    /* y is zero */
    return x;
  else if (dexp > MANTLENGTH_DP64 + 1 || dexp < -MANTLENGTH_DP64 - 1)
    /* One of x and y is insignificant compared to the other */
    return x + y; /* Raise inexact */
  else if (xexp > EXPBIAS_DP64 + 500 || yexp > EXPBIAS_DP64 + 500)
    {
      /* Danger of overflow; scale down by 2**600. */
      expadjust = 600;
      ux -= 0x2580000000000000;
      PUT_BITS_DP64(ux, x);
      uy -= 0x2580000000000000;
      PUT_BITS_DP64(uy, y);
    }
  else if (xexp < EXPBIAS_DP64 - 500 || yexp < EXPBIAS_DP64 - 500)
    {
      /* Danger of underflow; scale up by 2**600. */
      expadjust = -600;
      if (xexp == 0)
        {
          /* x is denormal - handle by adding 601 to the exponent
           and then subtracting a correction for the implicit bit */
          PUT_BITS_DP64(ux + 0x2590000000000000, x);
          x -= 9.23297861778573578076e-128; /* 0x2590000000000000 */
          GET_BITS_DP64(x, ux);
        }
      else
        {
          /* x is normal - just increase the exponent by 600 */
          ux += 0x2580000000000000;
          PUT_BITS_DP64(ux, x);
        }
      if (yexp == 0)
        {
          PUT_BITS_DP64(uy + 0x2590000000000000, y);
          y -= 9.23297861778573578076e-128; /* 0x2590000000000000 */
          GET_BITS_DP64(y, uy);
        }
      else
        {
          uy += 0x2580000000000000;
          PUT_BITS_DP64(uy, y);
        }
    }


#ifdef FAST_BUT_GREATER_THAN_ONE_ULP
  /* Not awful, but results in accuracy loss larger than 1 ulp */
  r = x*x + y*y;
#else
  /* Slower but more accurate */

  /* Sort so that x is greater than y */
  if (x < y)
    {
      u = y;
      y = x;
      x = u;
      ut = ux;
      ux = uy;
      uy = ut;
    }

  /* Split x into hx and tx, head and tail */
  PUT_BITS_DP64(ux & 0xfffffffff8000000, hx);
  tx = x - hx;

  PUT_BITS_DP64(uy & 0xfffffffff8000000, hy);
  ty = y - hy;

  /* Compute r = x*x + y*y with extra precision */
  x2 = x*x;
  y2 = y*y;
  hs = x2 + y2;

  if (dexp == 0)
    /* We take most care when x and y have equal exponents,
       i.e. are almost the same size */
    ts = (((x2 - hs) + y2) +
          ((hx * hx - x2) + 2 * hx * tx) + tx * tx) +
      ((hy * hy - y2) + 2 * hy * ty) + ty * ty;
  else
    ts = (((x2 - hs) + y2) +
          ((hx * hx - x2) + 2 * hx * tx) + tx * tx);

  r = hs + ts;
#endif

  /* The sqrt can introduce another half ulp error. */
  /* VC++ intrinsic call */
  _mm_store_sd(&retval, _mm_sqrt_sd(_mm_setzero_pd(), _mm_load_sd(&r)));

  /* If necessary scale the result back. This may lead to
     overflow but if so that's the correct result. */
  retval = scaleDouble_1(retval, expadjust);

  if (retval > large)
    /* The result overflowed. Deal with errno. */
    return _handle_error("_hypot", OP_HYPOT, PINFBITPATT_DP64, _OVERFLOW,
                        AMD_F_OVERFLOW | AMD_F_INEXACT, ERANGE, x, y, 2);

  return retval;
}
