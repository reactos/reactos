
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
#define USE_HANDLE_ERRORF
#include "libm_inlines.h"
#undef USE_HANDLE_ERRORF

// Disable "C4163: not available as intrinsic function" warning that older
// compilers may issue here.
#pragma warning(disable:4163)
#pragma function(floorf)

float FN_PROTOTYPE(floorf)(float x)
{
  float r;
  int rexp, xneg;
  unsigned int ux, ax, ur, mask;

  GET_BITS_SP32(x, ux);
  ax = ux & (~SIGNBIT_SP32);
  xneg = (ux != ax);

  if (ax >= 0x4b800000)
    {
      /* abs(x) is either NaN, infinity, or >= 2^24 */
      if (ax > 0x7f800000)
        /* x is NaN */
        return _handle_errorf("floorf", OP_FLOOR, ux|0x00400000, _DOMAIN,
                             0, EDOM, x, 0.0F, 1);
      else
        return x;
    }
  else if (ax < 0x3f800000) /* abs(x) < 1.0 */
    {
      if (ax == 0x00000000)
        /* x is +zero or -zero; return the same zero */
        return x;
      else if (xneg) /* x < 0.0 */
        return -1.0F;
      else
        return 0.0F;
    }
  else
    {
      rexp = ((ux & EXPBITS_SP32) >> EXPSHIFTBITS_SP32) - EXPBIAS_SP32;
      /* Mask out the bits of r that we don't want */
      mask = (1 << (EXPSHIFTBITS_SP32 - rexp)) - 1;
      ur = (ux & ~mask);
      PUT_BITS_SP32(ur, r);
      if (xneg && (ux != ur))
        /* We threw some bits away and x was negative */
        return r - 1.0F;
      else
        return r;
    }
}
