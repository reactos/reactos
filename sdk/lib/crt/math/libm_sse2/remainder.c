
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

#define USE_NAN_WITH_FLAGS
#define USE_SCALEDOUBLE_3
#define USE_GET_FPSW_INLINE
#define USE_SET_FPSW_INLINE
#define USE_HANDLE_ERROR
#include "libm_inlines.h"
#undef USE_NAN_WITH_FLAGS
#undef USE_SCALEDOUBLE_3
#undef USE_GET_FPSW_INLINE
#undef USE_SET_FPSW_INLINE
#undef USE_HANDLE_ERROR

#if !defined(_CRTBLD_C9X)
#define _CRTBLD_C9X
#endif

#include "libm_errno.h"

/* Computes the exact product of x and y, the result being the
   nearly doublelength number (z,zz) */
static inline void dekker_mul12(double x, double y,
                double *z, double *zz)
{
  double hx, tx, hy, ty;
  /* Split x into hx (head) and tx (tail). Do the same for y. */
  unsigned long long u;
  GET_BITS_DP64(x, u);
  u &= 0xfffffffff8000000;
  PUT_BITS_DP64(u, hx);
  tx = x - hx;
  GET_BITS_DP64(y, u);
  u &= 0xfffffffff8000000;
  PUT_BITS_DP64(u, hy);
  ty = y - hy;
  *z = x * y;
  *zz = (((hx * hy - *z) + hx * ty) + tx * hy) + tx * ty;
}

#pragma function(fmod)
#undef _FUNCNAME
#if defined(COMPILING_FMOD)
double fmod(double x, double y)
#define _FUNCNAME "fmod"
#define _OPERATION OP_FMOD
#else
double remainder(double x, double y)
#define _FUNCNAME "remainder"
#define _OPERATION OP_REM
#endif
{
  double dx, dy, scale, w, t, v, c, cc;
  int i, ntimes, xexp, yexp;
  unsigned long long u, ux, uy, ax, ay, todd;
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
      /* x or y is zero, denormalized, NaN or infinity */
      if (xexp > BIASEDEMAX_DP64)
        {
          /* x is NaN or infinity */
          if (ux & MANTBITS_DP64)
            {
              /* x is NaN */
              return _handle_error(_FUNCNAME, _OPERATION, ux|0x0008000000000000, _DOMAIN, 0,
                                  EDOM, x, y, 2);
            }
          else
            {
              /* x is infinity; result is NaN */
              return _handle_error(_FUNCNAME, _OPERATION, INDEFBITPATT_DP64, _DOMAIN,
                                  AMD_F_INVALID, EDOM, x, y, 2);
            }
        }
      else if (yexp > BIASEDEMAX_DP64)
        {
          /* y is NaN or infinity */
          if (uy & MANTBITS_DP64)
            {
              /* y is NaN */
              return _handle_error(_FUNCNAME, _OPERATION, uy|0x0008000000000000, _DOMAIN, 0,
                                  EDOM, x, y, 2);
            }
          else
            {
#ifdef _CRTBLD_C9X
              /* C99 return for y = +-inf is x */
              return x;
#else
              /* y is infinity; result is indefinite */
              return _handle_error(_FUNCNAME, _OPERATION, INDEFBITPATT_DP64, _DOMAIN,
                                  AMD_F_INVALID, EDOM, x, y, 2);
#endif
            }
        }
      else if (ax == 0x0000000000000000)
        {
          /* x is zero */
          if (ay == 0x0000000000000000)
            {
              /* y is zero */
              return _handle_error(_FUNCNAME, _OPERATION, INDEFBITPATT_DP64, _DOMAIN,
                                  AMD_F_INVALID, EDOM, x, y, 2);
            }
          else
            /* C99 return for x = 0 must preserve sign */
            return x;
        }
      else if (ay == 0x0000000000000000)
        {
          /* y is zero */
          return _handle_error(_FUNCNAME, _OPERATION, INDEFBITPATT_DP64, _DOMAIN,
                              AMD_F_INVALID, EDOM, x, y, 2);
        }

      /* We've exhausted all other possibilities. One or both of x and
         y must be denormalized */
      if (xexp < 1)
        {
          /* x is denormalized. Figure out its exponent. */
          u = ax;
          while (u < IMPBIT_DP64)
            {
              xexp--;
              u <<= 1;
            }
        }
      if (yexp < 1)
        {
          /* y is denormalized. Figure out its exponent. */
          u = ay;
          while (u < IMPBIT_DP64)
            {
              yexp--;
              u <<= 1;
            }
        }
    }
  else if (ax == ay)
    {
      /* abs(x) == abs(y); return zero with the sign of x */
      PUT_BITS_DP64(ux & SIGNBIT_DP64, dx);
      return dx;
    }

  /* Set x = abs(x), y = abs(y) */
  PUT_BITS_DP64(ax, dx);
  PUT_BITS_DP64(ay, dy);

  if (ax < ay)
    {
      /* abs(x) < abs(y) */
#if !defined(COMPILING_FMOD)
      if (dx > 0.5*dy)
        dx -= dy;
#endif
      return x < 0.0? -dx : dx;
    }

  /* Save the current floating-point status word. We need
     to do this because the remainder function is always
     exact for finite arguments, but our algorithm causes
     the inexact flag to be raised. We therefore need to
     restore the entry status before exiting. */
  sw = get_fpsw_inline();

  /* Set ntimes to the number of times we need to do a
     partial remainder. If the exponent of x is an exact multiple
     of 52 larger than the exponent of y, and the mantissa of x is
     less than the mantissa of y, ntimes will be one too large
     but it doesn't matter - it just means that we'll go round
     the loop below one extra time. */
  if (xexp <= yexp)
    ntimes = 0;
  else
    ntimes = (xexp - yexp) / 52;

  if (ntimes == 0)
    {
      w = dy;
      scale = 1.0;
    }
  else
    {
      /* Set w = y * 2^(52*ntimes) */
      w = scaleDouble_3(dy, ntimes * 52);

      /* Set scale = 2^(-52) */
      PUT_BITS_DP64((unsigned long long)(-52 + EXPBIAS_DP64) << EXPSHIFTBITS_DP64,
                    scale);
    }


  /* Each time round the loop we compute a partial remainder.
     This is done by subtracting a large multiple of w
     from x each time, where w is a scaled up version of y.
     The subtraction must be performed exactly in quad
     precision, though the result at each stage can
     fit exactly in a double precision number. */
  for (i = 0; i < ntimes; i++)
    {
      /* t is the integer multiple of w that we will subtract.
         We use a truncated value for t.

         N.B. w has been chosen so that the integer t will have
         at most 52 significant bits. This is the amount by
         which the exponent of the partial remainder dx gets reduced
         every time around the loop. In theory we could use
         53 bits in t, but the quad precision multiplication
         routine dekker_mul12 does not allow us to do that because
         it loses the last (106th) bit of its quad precision result. */

      /* Set dx = dx - w * t, where t is equal to trunc(dx/w). */
      t = (double)(long long)(dx / w);
      /* At this point, t may be one too large due to
         rounding of dx/w */

      /* Compute w * t in quad precision */
      dekker_mul12(w, t, &c, &cc);

      /* Subtract w * t from dx */
      v = dx - c;
      dx = v + (((dx - v) - c) - cc);

      /* If t was one too large, dx will be negative. Add back
         one w */
      /* It might be possible to speed up this loop by finding
         a way to compute correctly truncated t directly from dx and w.
         We would then avoid the need for this check on negative dx. */
      if (dx < 0.0)
        dx += w;

      /* Scale w down by 2^(-52) for the next iteration */
      w *= scale;
    }

  /* One more time */
  /* Variable todd says whether the integer t is odd or not */
  t = (double)(long long)(dx / w);
  todd = ((long long)(dx / w)) & 1;
  dekker_mul12(w, t, &c, &cc);
  v = dx - c;
  dx = v + (((dx - v) - c) - cc);
  if (dx < 0.0)
    {
      todd = !todd;
      dx += w;
    }

  /* At this point, dx lies in the range [0,dy) */
#if !defined(COMPILING_FMOD)
  /* For the fmod function, we're done apart from setting
     the correct sign. */
  /* For the remainder function, we need to adjust dx
     so that it lies in the range (-y/2, y/2] by carefully
     subtracting w (== dy == y) if necessary. The rigmarole
     with todd is to get the correct sign of the result
     when x/y lies exactly half way between two integers,
     when we need to choose the even integer. */
  if (ay < 0x7fd0000000000000)
    {
      if (dx + dx > w || (todd && (dx + dx == w)))
        dx -= w;
    }
  else if (dx > 0.5 * w || (todd && (dx == 0.5 * w)))
    dx -= w;

#endif

  /* **** N.B. for some reason this breaks the 32 bit version
     of remainder when compiling with optimization. */
  /* Restore the entry status flags */
  set_fpsw_inline(sw);

  /* Set the result sign according to input argument x */
  return x < 0.0? -dx : dx;

}
