
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

#if USE_SOFTWARE_SQRT
#define USE_SQRT_AMD_INLINE
#endif
#define USE_NAN_WITH_FLAGS
#define USE_HANDLE_ERROR
#include "libm_inlines.h"
#if USE_SOFTWARE_SQRT
#undef USE_SQRT_AMD_INLINE
#endif
#undef USE_NAN_WITH_FLAGS
#undef USE_HANDLE_ERROR

#include "libm_errno.h"

#pragma function(sqrt)

double sqrt(double x)
{
#if USE_SOFTWARE_SQRT
  return sqrt_amd_inline(x);
#else
  double r;
  unsigned long long ux;
  GET_BITS_DP64(x, ux);

  /* Check for special cases for Microsoft error handling */
  if ((ux & PINFBITPATT_DP64) == PINFBITPATT_DP64)
    {
      /* x is infinity, or NaN */
      if (ux & MANTBITS_DP64)
        {
          /* NaN  of some sort */
          /* If it's a signaling NaN, convert to QNaN */
          return _handle_error("sqrt", OP_SQRT, ux|0x0008000000000000, 
                                  _DOMAIN, 0,EDOM, x, 0.0, 1);
        }
      else
        {
          /* +/-infinity  */
          if (ux & SIGNBIT_DP64)
            {
              /* - infinity */
                return _handle_error("sqrt", OP_SQRT, INDEFBITPATT_DP64, 
                                    _DOMAIN, AMD_F_INVALID, EDOM, x, 0.0, 1);
            }
          /* positive infinite is not a problem */
        }
    }
  if ((ux & SIGNBIT_DP64)&&(ux & ~SIGNBIT_DP64))   /* if x < zero */
    {
        return _handle_error("sqrt", OP_SQRT, INDEFBITPATT_DP64, 
                            _DOMAIN, AMD_F_INVALID, EDOM, x, 0.0, 1);
    }

      /* VC++ intrinsic call */
  _mm_store_sd(&r, _mm_sqrt_sd(_mm_setzero_pd(), _mm_load_sd(&x)));
  return r;
#endif
}
