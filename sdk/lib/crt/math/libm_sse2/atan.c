
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

#define USE_VAL_WITH_FLAGS
#define USE_NAN_WITH_FLAGS
#define USE_HANDLE_ERROR
#include "libm_inlines.h"
#undef USE_VAL_WITH_FLAGS
#undef USE_NAN_WITH_FLAGS
#undef USE_HANDLE_ERROR

#include "libm_errno.h"

#ifdef _MSC_VER
#pragma function(atan)
#endif

double FN_PROTOTYPE(atan)(double x)
{

  /* Some constants and split constants. */

  static double piby2 = 1.5707963267948966e+00; /* 0x3ff921fb54442d18 */
  double chi, clo, v, s, q, z;

  /* Find properties of argument x. */

  unsigned long long ux, aux, xneg;
  GET_BITS_DP64(x, ux);
  aux = ux & ~SIGNBIT_DP64;
  xneg = (ux != aux);

  if (xneg) v = -x;
  else v = x;

  /* Argument reduction to range [-7/16,7/16] */

  if (aux > 0x4003800000000000) /* v > 39./16. */
    {

      if (aux > PINFBITPATT_DP64)
        {
          /* x is NaN */
          return _handle_error("atan", OP_ATAN, ux|0x0008000000000000, _DOMAIN, 0,
                              EDOM, x, 0.0, 1);
        }
      else if (v > 0x4370000000000000)
	{ /* abs(x) > 2^56 => arctan(1/x) is
	     insignificant compared to piby2 */
	  if (xneg)
            return val_with_flags(-piby2, AMD_F_INEXACT);
	  else
            return val_with_flags(piby2, AMD_F_INEXACT);
	}

      x = -1.0/v;
      /* (chi + clo) = arctan(infinity) */
      chi = 1.57079632679489655800e+00; /* 0x3ff921fb54442d18 */
      clo = 6.12323399573676480327e-17; /* 0x3c91a62633145c06 */
    }
  else if (aux > 0x3ff3000000000000) /* 39./16. > v > 19./16. */
    {
      x = (v-1.5)/(1.0+1.5*v);
      /* (chi + clo) = arctan(1.5) */
      chi = 9.82793723247329054082e-01; /* 0x3fef730bd281f69b */
      clo = 1.39033110312309953701e-17; /* 0x3c7007887af0cbbc */
    }
  else if (aux > 0x3fe6000000000000) /* 19./16. > v > 11./16. */
    {
      x = (v-1.0)/(1.0+v);
      /* (chi + clo) = arctan(1.) */
      chi = 7.85398163397448278999e-01; /* 0x3fe921fb54442d18 */
      clo = 3.06161699786838240164e-17; /* 0x3c81a62633145c06 */
    }
  else if (aux > 0x3fdc000000000000) /* 11./16. > v > 7./16. */
    {
      x = (2.0*v-1.0)/(2.0+v);
      /* (chi + clo) = arctan(0.5) */
      chi = 4.63647609000806093515e-01; /* 0x3fddac670561bb4f */
      clo = 2.26987774529616809294e-17; /* 0x3c7a2b7f222f65e0 */
    }
  else  /* v < 7./16. */
    {
      x = v;
      chi = 0.0;
      clo = 0.0;
    }

  /* Core approximation: Remez(4,4) on [-7/16,7/16] */

  s = x*x;
  q = x*s*
       (0.268297920532545909e0 +
	(0.447677206805497472e0 +
	 (0.220638780716667420e0 +
	  (0.304455919504853031e-1 +
	    0.142316903342317766e-3*s)*s)*s)*s)/
       (0.804893761597637733e0 +
	(0.182596787737507063e1 +
	 (0.141254259931958921e1 +
	  (0.424602594203847109e0 +
	    0.389525873944742195e-1*s)*s)*s)*s);

  z = chi - ((q - clo) - x);

  if (xneg) z = -z;
  return z;
}
