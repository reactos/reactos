
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

#include "libm_errno.h"
#define USE_HANDLE_ERROR
#include "libm_inlines.h"
#undef USE_HANDLE_ERROR

// Disable "C4163: not available as intrinsic function" warning that older
// compilers may issue here.
#pragma warning(disable:4163)
#pragma function(ceil)

double FN_PROTOTYPE(ceil)(double x)
{
  double r;
  long long rexp, xneg;
  unsigned long long ux, ax, ur, mask;

  GET_BITS_DP64(x, ux);
  ax = ux & (~SIGNBIT_DP64);
  xneg = (ux != ax);

  if (ax >= 0x4340000000000000)
    {
      /* abs(x) is either NaN, infinity, or >= 2^53 */
      if (ax > 0x7ff0000000000000)
        /* x is NaN */
        return _handle_error("ceil", OP_CEIL, ux|0x0008000000000000, _DOMAIN, 0,
                            EDOM, x, 0.0, 1);
      else
        return x;
    }
  else if (ax < 0x3ff0000000000000) /* abs(x) < 1.0 */
    {
      if (ax == 0x0000000000000000)
        /* x is +zero or -zero; return the same zero */
          return x;
      else if (xneg) /* x < 0.0 */
      {
        PUT_BITS_DP64(SIGNBIT_DP64, r);  /* return -0.0 */
        return r;
      }
      else
        return 1.0;
    }
  else
    {
      rexp = ((ux & EXPBITS_DP64) >> EXPSHIFTBITS_DP64) - EXPBIAS_DP64;
      /* Mask out the bits of r that we don't want */
      mask = 1;
      mask = (mask << (EXPSHIFTBITS_DP64 - rexp)) - 1;
      ur = (ux & ~mask);
      PUT_BITS_DP64(ur, r);
      if (xneg || (ur == ux))
        return r;
      else
        /* We threw some bits away and x was positive */
        return r + 1.0;
    }

}
