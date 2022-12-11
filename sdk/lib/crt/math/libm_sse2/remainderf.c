
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

#define USE_NANF_WITH_FLAGS
#define USE_SCALEDOUBLE_1
#define USE_GET_FPSW_INLINE
#define USE_SET_FPSW_INLINE
#define USE_HANDLE_ERRORF
#include "libm_inlines.h"
#undef USE_NANF_WITH_FLAGS
#undef USE_SCALEDOUBLE_1
#undef USE_GET_FPSW_INLINE
#undef USE_SET_FPSW_INLINE
#undef USE_HANDLE_ERRORF

#if !defined(_CRTBLD_C9X)
#define _CRTBLD_C9X
#endif

#include "libm_errno.h"

// Disable "C4163: not available as intrinsic function" warning that older
// compilers may issue here.
#pragma warning(disable:4163)
#pragma function(remainderf,fmodf)


#undef _FUNCNAME
#if defined(COMPILING_FMOD)
float fmodf(float x, float y)
#define _FUNCNAME "fmodf"
#define _OPERATION OP_FMOD
#else
float remainderf(float x, float y)
#define _FUNCNAME "remainderf"
#define _OPERATION OP_REM
#endif
{
  double dx, dy, scale, w, t;
  int i, ntimes, xexp, yexp;
  unsigned long long ux, uy, ax, ay;

  unsigned int sw;

  dx = x;
  dy = y;


  GET_BITS_DP64(dx, ux);
  GET_BITS_DP64(dy, uy);
  ax = ux & ~SIGNBIT_DP64;
  ay = uy & ~SIGNBIT_DP64;
  xexp = (int)((ux & EXPBITS_DP64) >> EXPSHIFTBITS_DP64);
  yexp = (int)((uy & EXPBITS_DP64) >> EXPSHIFTBITS_DP64);

  if (xexp < 1 || xexp > BIASEDEMAX_DP64 ||
      yexp < 1 || yexp > BIASEDEMAX_DP64)
    {
      /* x or y is zero, NaN or infinity (neither x nor y can be
         denormalized because we promoted from float to double) */
      if (xexp > BIASEDEMAX_DP64)
        {
          /* x is NaN or infinity */
          if (ux & MANTBITS_DP64)
            {
              /* x is NaN */
              unsigned int ufx;
              GET_BITS_SP32(x, ufx);
              return _handle_errorf(_FUNCNAME, _OPERATION, ufx|0x00400000, _DOMAIN, 0,
                                   EDOM, x, y, 2);
            }
          else
            {
              /* x is infinity; result is NaN */
              return _handle_errorf(_FUNCNAME, _OPERATION, INDEFBITPATT_SP32, _DOMAIN,
                                   AMD_F_INVALID, EDOM, x, y, 2);
            }
        }
      else if (yexp > BIASEDEMAX_DP64)
        {
          /* y is NaN or infinity */
          if (uy & MANTBITS_DP64)
            {
              /* y is NaN */
              unsigned int ufy;
              GET_BITS_SP32(y, ufy);
              return _handle_errorf(_FUNCNAME, _OPERATION, ufy|0x00400000, _DOMAIN, 0,
                                   EDOM, x, y, 2);
            }
          else
            {
#ifdef _CRTBLD_C9X
              /* C99 return for y = +-inf is x */
              return x;
#else
              /* y is infinity; result is indefinite */
              return _handle_errorf(_FUNCNAME, _OPERATION, INDEFBITPATT_SP32, _DOMAIN,
                                   AMD_F_INVALID, EDOM, x, y, 2);
#endif
            }
        }
      else if (xexp < 1)
        {
          /* x must be zero (cannot be denormalized) */
          if (yexp < 1)
            {
              /* y must be zero (cannot be denormalized) */
              return _handle_errorf(_FUNCNAME, _OPERATION, INDEFBITPATT_SP32, _DOMAIN,
                                   AMD_F_INVALID, EDOM, x, y, 2);
            }
          else
              /* C99 return for x = 0 must preserve sign */
              return x;
        }
      else
        {
          /* y must be zero */
          return _handle_errorf(_FUNCNAME, _OPERATION, INDEFBITPATT_SP32, _DOMAIN,
                               AMD_F_INVALID, EDOM, x, y, 2);
        }
    }
  else if (ax == ay)
    {
      /* abs(x) == abs(y); return zero with the sign of x */
      PUT_BITS_DP64(ux & SIGNBIT_DP64, dx);
      return (float)dx;
    }

  /* Set dx = abs(x), dy = abs(y) */
  PUT_BITS_DP64(ax, dx);
  PUT_BITS_DP64(ay, dy);

  if (ax < ay)
    {
      /* abs(x) < abs(y) */
#if !defined(COMPILING_FMOD)
      if (dx > 0.5*dy)
        dx -= dy;
#endif
      return (float)(x < 0.0? -dx : dx);
    }

  /* Save the current floating-point status word. We need
     to do this because the remainder function is always
     exact for finite arguments, but our algorithm causes
     the inexact flag to be raised. We therefore need to
     restore the entry status before exiting. */
  sw = get_fpsw_inline();

  /* Set ntimes to the number of times we need to do a
     partial remainder. If the exponent of x is an exact multiple
     of 24 larger than the exponent of y, and the mantissa of x is
     less than the mantissa of y, ntimes will be one too large
     but it doesn't matter - it just means that we'll go round
     the loop below one extra time. */
  if (xexp <= yexp)
    {
      ntimes = 0;
      w = dy;
      scale = 1.0;
    }
  else
    {
      ntimes = (xexp - yexp) / 24;

      /* Set w = y * 2^(24*ntimes) */
      PUT_BITS_DP64((unsigned long long)(ntimes * 24 + EXPBIAS_DP64) << EXPSHIFTBITS_DP64,
                    scale);
      w = scale * dy;
      /* Set scale = 2^(-24) */
      PUT_BITS_DP64((unsigned long long)(-24 + EXPBIAS_DP64) << EXPSHIFTBITS_DP64,
                    scale);
    }


  /* Each time round the loop we compute a partial remainder.
     This is done by subtracting a large multiple of w
     from x each time, where w is a scaled up version of y.
     The subtraction can be performed exactly when performed
     in double precision, and the result at each stage can
     fit exactly in a single precision number. */
  for (i = 0; i < ntimes; i++)
    {
      /* t is the integer multiple of w that we will subtract.
         We use a truncated value for t. */
      t = (double)((int)(dx / w));
      dx -= w * t;
      /* Scale w down by 2^(-24) for the next iteration */
      w *= scale;
    }

  /* One more time */
#if defined(COMPILING_FMOD)
  t = (double)((int)(dx / w));
  dx -= w * t;
#else
 {
  unsigned int todd;
  /* Variable todd says whether the integer t is odd or not */
  t = (double)((int)(dx / w));
  todd = ((int)(dx / w)) & 1;
  dx -= w * t;

  /* At this point, dx lies in the range [0,dy) */
  /* For the remainder function, we need to adjust dx
     so that it lies in the range (-y/2, y/2] by carefully
     subtracting w (== dy == y) if necessary. */
  if (dx > 0.5 * w || ((dx == 0.5 * w) && todd))
    dx -= w;
 }
#endif

  /* **** N.B. for some reason this breaks the 32 bit version
     of remainder when compiling with optimization. */
  /* Restore the entry status flags */
  set_fpsw_inline(sw);

  /* Set the result sign according to input argument x */
  return (float)(x < 0.0? -dx : dx);

}
