
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

#define USE_SPLITEXP
#define USE_SCALEDOUBLE_1
#define USE_SCALEDOUBLE_2
#define USE_ZERO_WITH_FLAGS
#define USE_INFINITY_WITH_FLAGS
#define USE_HANDLE_ERROR

#include "libm_inlines.h"
#undef USE_ZERO_WITH_FLAGS
#undef USE_SPLITEXP
#undef USE_SCALEDOUBLE_1
#undef USE_SCALEDOUBLE_2
#undef USE_INFINITY_WITH_FLAGS
#undef USE_HANDLE_ERROR

#include "libm_errno.h"

double FN_PROTOTYPE(exp2)(double x)
{
  static const double
    max_exp2_arg = 1024.0,  /* 0x4090000000000000 */
    min_exp2_arg = -1074.0, /* 0xc090c80000000000 */
    log2 = 6.931471805599453094178e-01, /* 0x3fe62e42fefa39ef */
    log2_lead = 6.93147167563438415527E-01, /* 0x3fe62e42f8000000 */
    log2_tail = 1.29965068938898869640E-08, /* 0x3e4be8e7bcd5e4f1 */
    one_by_32_lead = 0.03125;

  double y, z1, z2, z, hx, tx, y1, y2;
  int m;
  unsigned long long ux, ax;

  /*
    Computation of exp2(x).

    We compute the values m, z1, and z2 such that
    exp2(x) = 2**m * (z1 + z2),  where exp2(x) is 2**x.

    Computations needed in order to obtain m, z1, and z2
    involve three steps.

    First, we reduce the argument x to the form
    x = n/32 + remainder,
    where n has the value of an integer and |remainder| <= 1/64.
    The value of n = x * 32 rounded to the nearest integer and
    the remainder = x - n/32.

    Second, we approximate exp2(r1 + r2) - 1 where r1 is the leading
    part of the remainder and r2 is the trailing part of the remainder.

    Third, we reconstruct exp2(x) so that
    exp2(x) = 2**m * (z1 + z2).
  */


  GET_BITS_DP64(x, ux);
  ax = ux & (~SIGNBIT_DP64);

  if (ax >= 0x4090000000000000) /* abs(x) >= 1024.0 */
    {
      if(ax >= 0x7ff0000000000000)
        {
          /* x is either NaN or infinity */
          if (ux & MANTBITS_DP64)
            /* x is NaN */
            return _handle_error("exp2", OP_EXP, ux|0x0008000000000000, _DOMAIN,
                                0, EDOM, x, 0.0, 1);
          else if (ux & SIGNBIT_DP64)
            /* x is negative infinity; return 0.0 with no flags. */
            return 0.0;
          else
            /* x is positive infinity */
            return x;
        }
      if (x > max_exp2_arg)
        /* Return +infinity with overflow flag */
        return _handle_error("exp2", OP_EXP, PINFBITPATT_DP64, _OVERFLOW,
                            AMD_F_OVERFLOW | AMD_F_INEXACT, ERANGE, x, 0.0, 1);
      else if (x < min_exp2_arg)
        /* x is negative. Return +zero with underflow and inexact flags */
        return _handle_error("exp2", OP_EXP, 0, _UNDERFLOW,
                            AMD_F_UNDERFLOW | AMD_F_INEXACT, ERANGE, x, 0.0, 1);
    }


  /* Handle small arguments separately */
  if (ax < 0x3fb7154764ee6c2f)   /* abs(x) < 1/(16*log2) */
    {
      if (ax < 0x3c00000000000000)   /* abs(x) < 2^(-63) */
        return 1.0 + x; /* Raises inexact if x is non-zero */
      else
        {
          /* Split x into hx (head) and tx (tail). */
          unsigned long long u;
          hx = x;
          GET_BITS_DP64(hx, u);
          u &= 0xfffffffff8000000;
          PUT_BITS_DP64(u, hx);
          tx = x - hx;
          /* Carefully multiply x by log2. y1 is the most significant
             part of the result, and y2 the least significant part */
          y1 = x * log2_lead;
          y2 = (((hx * log2_lead - y1) + hx * log2_tail) +
                  tx * log2_lead) + tx * log2_tail;

          y = y1 + y2;
		z = (9.99564649780173690e-1 +
		     (1.61251249355268050e-5 +
		      (2.37986978239838493e-2 +
		        2.68724774856111190e-7*y)*y)*y)/
		    (9.99564649780173692e-1 +
		     (-4.99766199765151309e-1 +
		      (1.070876894098586184e-1 +
		       (-1.189773642681502232e-2 +
			 5.9480622371960190616e-4*y)*y)*y)*y);
          z = ((z * y1) + (z * y2)) + 1.0;
        }
    }
  else
    {
      /* Find m, z1 and z2 such that exp2(x) = 2**m * (z1 + z2) */

      splitexp(x, log2, 32.0, one_by_32_lead, 0.0, &m, &z1, &z2);

      /* Scale (z1 + z2) by 2.0**m */
      if (m > EMIN_DP64 && m < EMAX_DP64)
	z = scaleDouble_1((z1+z2),m);
      else
	z = scaleDouble_2((z1+z2),m);
    }
  return z;
}
