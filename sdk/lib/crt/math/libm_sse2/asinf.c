
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

#define USE_VALF_WITH_FLAGS
#define USE_NANF_WITH_FLAGS
#define USE_HANDLE_ERRORF
#include "libm_inlines.h"
#undef USE_NANF_WITH_FLAGS
#undef USE_VALF_WITH_FLAGS
#undef USE_HANDLE_ERRORF

#include "libm_errno.h"

#ifdef _MSC_VER
// Disable "C4163: not available as intrinsic function" warning that older
// compilers may issue here.
#pragma warning(disable:4163)
#pragma function(asinf)
#endif

float FN_PROTOTYPE(asinf)(float x)
{
  /* Computes arcsin(x).
     The argument is first reduced by noting that arcsin(x)
     is invalid for abs(x) > 1 and arcsin(-x) = -arcsin(x).
     For denormal and small arguments arcsin(x) = x to machine
     accuracy. Remaining argument ranges are handled as follows.
     For abs(x) <= 0.5 use
     arcsin(x) = x + x^3*R(x^2)
     where R(x^2) is a rational minimax approximation to
     (arcsin(x) - x)/x^3.
     For abs(x) > 0.5 exploit the identity:
      arcsin(x) = pi/2 - 2*arcsin(sqrt(1-x)/2)
     together with the above rational approximation, and
     reconstruct the terms carefully.
    */

  /* Some constants and split constants. */

  static const float
    piby2_tail  = 7.5497894159e-08F, /* 0x33a22168 */
    hpiby2_head = 7.8539812565e-01F, /* 0x3f490fda */
    piby2       = 1.5707963705e+00F; /* 0x3fc90fdb */
  float u, v, y, s = 0.0F, r;
  int xexp, xnan, transform = 0;

  unsigned int ux, aux, xneg;
  GET_BITS_SP32(x, ux);
  aux = ux & ~SIGNBIT_SP32;
  xneg = (ux & SIGNBIT_SP32);
  xnan = (aux > PINFBITPATT_SP32);
  xexp = (int)((ux & EXPBITS_SP32) >> EXPSHIFTBITS_SP32) - EXPBIAS_SP32;

  /* Special cases */

  if (xnan)
    {
      return _handle_errorf("asinf", OP_ASIN, ux|0x00400000, _DOMAIN, 0,
                           EDOM, x, 0.0F, 1);
    }
  else if (xexp < -14)
    /* y small enough that arcsin(x) = x */
    return valf_with_flags(x, AMD_F_INEXACT);
  else if (xexp >= 0)
    {
      /* abs(x) >= 1.0 */
      if (x == 1.0F)
        return valf_with_flags(piby2, AMD_F_INEXACT);
      else if (x == -1.0F)
        return valf_with_flags(-piby2, AMD_F_INEXACT);
      else
        return _handle_errorf("asinf", OP_ASIN, INDEFBITPATT_SP32, _DOMAIN,
                             AMD_F_INVALID, EDOM, x, 0.0F, 1);
    }

  if (xneg) y = -x;
  else y = x;

  transform = (xexp >= -1); /* abs(x) >= 0.5 */

  if (transform)
    { /* Transform y into the range [0,0.5) */
      r = 0.5F*(1.0F - y);
      /* VC++ intrinsic call */
      _mm_store_ss(&s, _mm_sqrt_ss(_mm_load_ss(&r)));
      y = s;
    }
  else
    r = y*y;

  /* Use a rational approximation for [0.0, 0.5] */

  u=r*(0.184161606965100694821398249421F +
       (-0.0565298683201845211985026327361F +
	(-0.0133819288943925804214011424456F -
	 0.00396137437848476485201154797087F*r)*r)*r)/
    (1.10496961524520294485512696706F -
     0.836411276854206731913362287293F*r);

  if (transform)
    {
      /* Reconstruct asin carefully in transformed region */
      float c, s1, p, q;
      unsigned int us;
      GET_BITS_SP32(s, us);
      PUT_BITS_SP32(0xffff0000 & us, s1);
      c = (r-s1*s1)/(s+s1);
      p = 2.0F*s*u - (piby2_tail-2.0F*c);
      q = hpiby2_head - 2.0F*s1;
      v = hpiby2_head - (p-q);
    }
  else
    {
      /* Use a temporary variable to prevent VC++ rearranging
            y + y*u
         into
            y * (1 + u)
         and getting an incorrectly rounded result */
      float tmp;
      tmp = y * u;
      v = y + tmp;
    }

  if (xneg) return -v;
  else return v;
}
