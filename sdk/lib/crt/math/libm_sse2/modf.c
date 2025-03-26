
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

double modf(double x, double *iptr)
{
  /* modf splits the argument x into integer and fraction parts,
     each with the same sign as x. */


  long long xexp;
  unsigned long long ux, ax, mask;

  GET_BITS_DP64(x, ux);
  ax = ux & (~SIGNBIT_DP64);

  if (ax >= 0x4340000000000000)
    {
      /* abs(x) is either NaN, infinity, or >= 2^53 */
      if (ax > 0x7ff0000000000000)
        {
          /* x is NaN */
          *iptr = x;
          return x + x; /* Raise invalid if it is a signalling NaN */
        }
      else
        {
          /* x is infinity or large. Return zero with the sign of x */
          *iptr = x;
          PUT_BITS_DP64(ux & SIGNBIT_DP64, x);
          return x;
        }
    }
  else if (ax < 0x3ff0000000000000)
    {
      /* abs(x) < 1.0. Set iptr to zero with the sign of x
         and return x. */
      PUT_BITS_DP64(ux & SIGNBIT_DP64, *iptr);
      return x;
    }
  else
    {
      xexp = ((ux & EXPBITS_DP64) >> EXPSHIFTBITS_DP64) - EXPBIAS_DP64;
      /* Mask out the bits of x that we don't want */
      mask = 1;
      mask = (mask << (EXPSHIFTBITS_DP64 - xexp)) - 1;
      PUT_BITS_DP64(ux & ~mask, *iptr);
      return x - *iptr;
    }

}
