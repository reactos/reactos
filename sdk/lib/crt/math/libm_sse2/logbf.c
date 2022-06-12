
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

#define USE_INFINITYF_WITH_FLAGS
#define USE_HANDLE_ERRORF
#include "libm_inlines.h"
#undef USE_INFINITYF_WITH_FLAGS
#undef USE_HANDLE_ERRORF

#include "libm_errno.h"

float _logbf(float x)
{
  unsigned int ux;
  int u;
  GET_BITS_SP32(x, ux);
  u = ((ux & EXPBITS_SP32) >> EXPSHIFTBITS_SP32) - EXPBIAS_SP32;
  if ((ux & ~SIGNBIT_SP32) == 0)
    /* x is +/-zero. Return -infinity with div-by-zero flag. */
    return _handle_errorf("_logbf", OP_LOGB, NINFBITPATT_SP32, _SING,
                         AMD_F_DIVBYZERO, ERANGE, x, 0.0F, 1);
  else if (EMIN_SP32 <= u && u <= EMAX_SP32)
    /* x is a normal number */
    return (float)u;
  else if (u > EMAX_SP32)
    {
      /* x is infinity or NaN */
      if ((ux & MANTBITS_SP32) == 0)
        /* x is +/-infinity. For VC++, return infinity of same sign. */
        return x;
      else
        /* x is NaN, result is NaN */
        return _handle_errorf("_logbf", OP_LOGB, ux|0x00400000, _DOMAIN,
                             0, EDOM, x, 0.0F, 1);
    }
  else
    {
      /* x is denormalized. */
#ifdef FOLLOW_IEEE754_LOGB
      /* Return the value of the minimum exponent to ensure that
         the relationship between logb and scalb, defined in
         IEEE 754, holds. */
      return EMIN_SP32;
#else
      /* Follow the rule set by IEEE 854 for logb */
      ux &= MANTBITS_SP32;
      u = EMIN_SP32;
      while (ux < IMPBIT_SP32)
        {
          ux <<= 1;
          u--;
        }
      return (float)u;
#endif
    }
}
