
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

#ifdef USE_SOFTWARE_SQRT
#define USE_SQRTF_AMD_INLINE
#endif
#define USE_INFINITYF_WITH_FLAGS
#define USE_HANDLE_ERRORF
#include "libm_inlines.h"
#ifdef USE_SOFTWARE_SQRT
#undef USE_SQRTF_AMD_INLINE
#endif
#undef USE_INFINITYF_WITH_FLAGS
#undef USE_HANDLE_ERRORF

#include "libm_errno.h"


float FN_PROTOTYPE(_hypotf)(float x, float y)
{
  /* Returns sqrt(x*x + y*y) with no overflow or underflow unless
     the result warrants it */

    /* Do intermediate computations in double precision
       and use sqrt instruction from chip if available. */
    double dx = x, dy = y, dr, retval;

    /* The largest finite float, stored as a double */
    const double large = 3.40282346638528859812e+38; /* 0x47efffffe0000000 */


  unsigned long long ux, uy, avx, avy;

  GET_BITS_DP64(x, avx);
  avx &= ~SIGNBIT_DP64;
  GET_BITS_DP64(y, avy);
  avy &= ~SIGNBIT_DP64;
  ux = (avx >> EXPSHIFTBITS_DP64);
  uy = (avy >> EXPSHIFTBITS_DP64);

  if (ux == BIASEDEMAX_DP64 + 1 || uy == BIASEDEMAX_DP64 + 1)
    {
      retval = x*x + y*y;
      /* One or both of the arguments are NaN or infinity. The
         result will also be NaN or infinity. */
      if (((ux == BIASEDEMAX_DP64 + 1) && !(avx & MANTBITS_DP64)) ||
          ((uy == BIASEDEMAX_DP64 + 1) && !(avy & MANTBITS_DP64)))
        /* x or y is infinity. ISO C99 defines that we must
           return +infinity, even if the other argument is NaN.
           Note that the computation of x*x + y*y above will already
           have raised invalid if either x or y is a signalling NaN. */
        return infinityf_with_flags(0);
      else
        /* One or both of x or y is NaN, and neither is infinity.
           Raise invalid if it's a signalling NaN */
        return (float)retval;
    }

    dr = (dx*dx + dy*dy);

#if USE_SOFTWARE_SQRT
    retval = sqrtf_amd_inline(r);
#else
  /* VC++ intrinsic call */
  _mm_store_sd(&retval, _mm_sqrt_sd(_mm_setzero_pd(), _mm_load_sd(&dr)));
#endif

    if (retval > large)
      return _handle_errorf("_hypotf", OP_HYPOT, PINFBITPATT_SP32, _OVERFLOW,
                           AMD_F_OVERFLOW | AMD_F_INEXACT, ERANGE, x, y, 2);
    else
      return (float)retval;
}
