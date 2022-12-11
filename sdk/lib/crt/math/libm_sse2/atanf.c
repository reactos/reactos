
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
#define USE_NAN_WITH_FLAGS
#define USE_HANDLE_ERRORF
#include "libm_inlines.h"
#undef USE_VALF_WITH_FLAGS
#undef USE_NAN_WITH_FLAGS
#undef USE_HANDLE_ERRORF

#include "libm_errno.h"

#ifdef _MSC_VER
// Disable "C4163: not available as intrinsic function" warning that older
// compilers may issue here.
#pragma warning(disable:4163)
#pragma function(atanf)
#endif

float FN_PROTOTYPE(atanf)(float fx)
{

  /* Some constants and split constants. */

  static double piby2 = 1.5707963267948966e+00; /* 0x3ff921fb54442d18 */

  double c, v, s, q, z;
  unsigned int xnan;

  double x = fx;

  /* Find properties of argument fx. */

  unsigned long long ux, aux, xneg;

  GET_BITS_DP64(x, ux);
  aux = ux & ~SIGNBIT_DP64;
  xneg = ux & SIGNBIT_DP64;

  v = x;
  if (xneg) v = -x;

  /* Argument reduction to range [-7/16,7/16] */

  if (aux < 0x3fdc000000000000) /* v < 7./16. */
    {
      x = v;
      c = 0.0;
    }
  else if (aux < 0x3fe6000000000000) /* v < 11./16. */
    {
      x = (2.0*v-1.0)/(2.0+v);
      /* c = arctan(0.5) */
      c = 4.63647609000806093515e-01; /* 0x3fddac670561bb4f */
    }
  else if (aux < 0x3ff3000000000000) /* v < 19./16. */
    {
      x = (v-1.0)/(1.0+v);
      /* c = arctan(1.) */
      c = 7.85398163397448278999e-01; /* 0x3fe921fb54442d18 */
    }
  else if (aux < 0x4003800000000000) /* v < 39./16. */
    {
      x = (v-1.5)/(1.0+1.5*v);
      /* c = arctan(1.5) */
      c = 9.82793723247329054082e-01; /* 0x3fef730bd281f69b */
    }
  else
    {

      xnan = (aux > PINFBITPATT_DP64);

      if (xnan)
        {
          /* x is NaN */
          unsigned int uhx;
          GET_BITS_SP32(fx, uhx);
          return _handle_errorf("atanf", OP_ATAN, uhx|0x00400000, _DOMAIN,
                               0, EDOM, fx, 0.0F, 1);
        }
      else if (v > 0x4c80000000000000)
	{ /* abs(x) > 2^26 => arctan(1/x) is
	     insignificant compared to piby2 */
	  if (xneg)
            return valf_with_flags((float)-piby2, AMD_F_INEXACT);
	  else
            return valf_with_flags((float)piby2, AMD_F_INEXACT);
	}

      x = -1.0/v;
      /* c = arctan(infinity) */
      c = 1.57079632679489655800e+00; /* 0x3ff921fb54442d18 */
    }

  /* Core approximation: Remez(2,2) on [-7/16,7/16] */

  s = x*x;
  q = x*s*
    (0.296528598819239217902158651186e0 +
     (0.192324546402108583211697690500e0 +
       0.470677934286149214138357545549e-2*s)*s)/
    (0.889585796862432286486651434570e0 +
     (0.111072499995399550138837673349e1 +
       0.299309699959659728404442796915e0*s)*s);

  z = c - (q - x);

  if (xneg) z = -z;
  return (float)z;
}
