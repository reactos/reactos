
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

#define USE_HANDLE_ERRORF
#define USE_SPLITEXPF
#define USE_SCALEFLOAT_2
#define USE_VALF_WITH_FLAGS
#include "libm_inlines.h"
#undef USE_SPLITEXPF
#undef USE_SCALEFLOAT_2
#undef USE_VALF_WITH_FLAGS
#undef USE_HANDLE_ERRORF

#include "libm_errno.h"

#ifdef _MSC_VER
// Disable "C4163: not available as intrinsic function" warning that older
// compilers may issue here.
#pragma warning(disable:4163)
#pragma function(tanhf)
#endif

float tanhf(float x)
{
  /*
    The definition of tanh(x) is sinh(x)/cosh(x), which is also equivalent
    to the following three formulae:
    1.  (exp(x) - exp(-x))/(exp(x) + exp(-x))
    2.  (1 - (2/(exp(2*x) + 1 )))
    3.  (exp(2*x) - 1)/(exp(2*x) + 1)
    but computationally, some formulae are better on some ranges.
  */
  static const float
    thirtytwo_by_log2 =  4.6166240692e+01F, /* 0x4238aa3b */
    log2_by_32_lead =  2.1659851074e-02F, /* 0x3cb17000 */
    log2_by_32_tail =  9.9831822808e-07F, /* 0x3585fdf4 */
    large_threshold = 10.0F; /* 0x41200000 */

  unsigned int ux, aux;
  float y, z, p, z1, z2, xneg;
  int m;

  /* Special cases */

  GET_BITS_SP32(x, ux);
  aux = ux & ~SIGNBIT_SP32;
  if (aux < 0x39000000) /* |x| small enough that tanh(x) = x */
    {
      if (aux == 0)
        return x; /* with no inexact */
      else
        return valf_with_flags(x, AMD_F_INEXACT);
    }
  else if (aux > 0x7f800000) /* |x| is NaN */
  {
      unsigned int ufx;
      GET_BITS_SP32(x, ufx);
      return _handle_errorf("tanhf", OP_TANH, ufx|0x00400000, _DOMAIN, 0,
                           EDOM, x, 0.0F, 1);
  }
//    return x + x;

  xneg = 1.0F - 2.0F * (aux != ux);

  y = xneg * x;

  if (y > large_threshold)
    {
      /* If x is large then exp(-x) is negligible and
         formula 1 reduces to plus or minus 1.0 */
      z = 1.0F;
    }
  else if (y <= 1.0F)
    {
      float y2;
      y2 = y*y;

      if (y < 0.9F)
        {
          /* Use a [2,1] Remez approximation on [0,0.9]. */
          z = y + y*y2*
            (-0.28192806108402678e0F +
             (-0.14628356048797849e-2F +
              0.4891631088530669873e-4F*y2)*y2)/
            (0.845784192581041099e0F +
             0.3427017942262751343e0F*y2);
        }
      else
        {
          /* Use a [2,1] Remez approximation on [0.9,1]. */
          z = y + y*y2*
            (-0.24069858695196524e0F +
             (-0.12325644183611929e-2F +
              0.3827534993599483396e-4F*y2)*y2)/
            (0.72209738473684982e0F +
             0.292529068698052819e0F*y2);
        }
    }
  else
    {
      /* Compute p = exp(2*y) + 1. The code is basically inlined
         from exp_amd. */

      splitexpf(2*y, 1.0F, thirtytwo_by_log2, log2_by_32_lead,
	       log2_by_32_tail, &m, &z1, &z2);
      p = scaleFloat_2(z1 + z2, m) + 1.0F;
      /* Now reconstruct tanh from p. */
      z = (1.0F - 2.0F/p);
    }

  return xneg * z;
}
