
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

#define USE_INFINITY_WITH_FLAGS
#define USE_HANDLE_ERROR
#include "libm_inlines.h"
#undef USE_INFINITY_WITH_FLAGS
#undef USE_HANDLE_ERROR

#include "libm_errno.h"

double _logb(double x)
{

  unsigned long long ux;
  long long u;
  GET_BITS_DP64(x, ux);
  u = ((ux & EXPBITS_DP64) >> EXPSHIFTBITS_DP64) - EXPBIAS_DP64;
  if ((ux & ~SIGNBIT_DP64) == 0)
    /* x is +/-zero. Return -infinity with div-by-zero flag. */
    return _handle_error("_logb", OP_LOGB, NINFBITPATT_DP64, _SING,
                        AMD_F_DIVBYZERO, ERANGE, x, 0.0, 1);
  else if (EMIN_DP64 <= u && u <= EMAX_DP64)
    /* x is a normal number */
    return (double)u;
  else if (u > EMAX_DP64)
    {
      /* x is infinity or NaN */
      if ((ux & MANTBITS_DP64) == 0)
        /* x is +/-infinity. For VC++, return infinity of same sign. */
        return x;
      else
        /* x is NaN, result is NaN */
        return _handle_error("_logb", OP_LOGB, ux|0x0008000000000000, _DOMAIN,
                            0, EDOM, x, 0.0, 1);
    }
  else
    {
      /* x is denormalized. */
#ifdef FOLLOW_IEEE754_LOGB
      /* Return the value of the minimum exponent to ensure that
         the relationship between logb and scalb, defined in
         IEEE 754, holds. */
      return EMIN_DP64;
#else
      /* Follow the rule set by IEEE 854 for logb */
      ux &= MANTBITS_DP64;
      u = EMIN_DP64;
      while (ux < IMPBIT_DP64)
        {
          ux <<= 1;
          u--;
        }
      return (double)u;
#endif
    }

}
