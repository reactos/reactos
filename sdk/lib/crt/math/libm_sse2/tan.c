
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

#define USE_NAN_WITH_FLAGS
#define USE_VAL_WITH_FLAGS
#define USE_HANDLE_ERROR
#include "libm_inlines.h"
#undef USE_NAN_WITH_FLAGS
#undef USE_VAL_WITH_FLAGS
#undef USE_HANDLE_ERROR

#include "libm_errno.h"

/* tan(x + xx) approximation valid on the interval [-pi/4,pi/4].
   If recip is true return -1/tan(x + xx) instead. */
static inline double tan_piby4(double x, double xx, int recip)
{
  double r, t1, t2, xl;
  int transform = 0;
  static const double
     piby4_lead = 7.85398163397448278999e-01, /* 0x3fe921fb54442d18 */
     piby4_tail = 3.06161699786838240164e-17; /* 0x3c81a62633145c06 */

  /* In order to maintain relative precision transform using the identity:
     tan(pi/4-x) = (1-tan(x))/(1+tan(x)) for arguments close to pi/4.
     Similarly use tan(x-pi/4) = (tan(x)-1)/(tan(x)+1) close to -pi/4. */

  if (x > 0.68)
    {
      transform = 1;
      x = piby4_lead - x;
      xl = piby4_tail - xx;
      x += xl;
      xx = 0.0;
    }
  else if (x < -0.68)
    {
      transform = -1;
      x = piby4_lead + x;
      xl = piby4_tail + xx;
      x += xl;
      xx = 0.0;
    }

  /* Core Remez [2,3] approximation to tan(x+xx) on the
     interval [0,0.68]. */

  r = x*x + 2.0 * x * xx;
  t1 = x;
  t2 = xx + x*r*
    (0.372379159759792203640806338901e0 +
     (-0.229345080057565662883358588111e-1 +
      0.224044448537022097264602535574e-3*r)*r)/
    (0.111713747927937668539901657944e1 +
     (-0.515658515729031149329237816945e0 +
      (0.260656620398645407524064091208e-1 -
       0.232371494088563558304549252913e-3*r)*r)*r);

  /* Reconstruct tan(x) in the transformed case. */

  if (transform)
    {
      double t;
      t = t1 + t2;
      if (recip)
         return transform*(2*t/(t-1) - 1.0);
      else
         return transform*(1.0 - 2*t/(1+t));
    }

  if (recip)
    {
      /* Compute -1.0/(t1 + t2) accurately */
      double trec, trec_top, z1, z2, t;
      unsigned long long u;
      t = t1 + t2;
      GET_BITS_DP64(t, u);
      u &= 0xffffffff00000000;
      PUT_BITS_DP64(u, z1);
      z2 = t2 - (z1 - t1);
      trec = -1.0 / t;
      GET_BITS_DP64(trec, u);
      u &= 0xffffffff00000000;
      PUT_BITS_DP64(u, trec_top);
      return trec_top + trec * ((1.0 + trec_top * z1) + trec_top * z2);

    }
  else
    return t1 + t2;
}

#ifdef _MSC_VER
#pragma function(tan)
#endif

double tan(double x)
{
  double r, rr;
  int region, xneg;

  unsigned long long ux, ax;
  GET_BITS_DP64(x, ux);
  ax = (ux & ~SIGNBIT_DP64);
  if (ax <= 0x3fe921fb54442d18) /* abs(x) <= pi/4 */
    {
      if (ax < 0x3f20000000000000) /* abs(x) < 2.0^(-13) */
        {
          if (ax < 0x3e40000000000000) /* abs(x) < 2.0^(-27) */
	    {
	      if (ax == 0x0000000000000000) return x;
              else return val_with_flags(x, AMD_F_INEXACT);
	    }
          else
            {
              /* Using a temporary variable prevents 64-bit VC++ from
                 rearranging
                    x + x*x*x*0.333333333333333333;
                 into
                    x * (1 + x*x*0.333333333333333333);
                 The latter results in an incorrectly rounded answer. */
              double tmp;
              tmp = x*x*x*0.333333333333333333;
              return x + tmp;
            }
        }
      else
        return tan_piby4(x, 0.0, 0);
    }
  else if ((ux & EXPBITS_DP64) == EXPBITS_DP64)
    {
      /* x is either NaN or infinity */
      if (ux & MANTBITS_DP64)
        /* x is NaN */
        return _handle_error("tan", OP_TAN, ux|0x0008000000000000, _DOMAIN, 0,
                            EDOM, x, 0.0, 1);
      else
        /* x is infinity. Return a NaN */
        return _handle_error("tan", OP_TAN, INDEFBITPATT_DP64, _DOMAIN, AMD_F_INVALID,
                            EDOM, x, 0.0, 1);
    }
  xneg = (ax != ux);


  if (xneg)
    x = -x;

  if (x < 5.0e5)
    {
      /* For these size arguments we can just carefully subtract the
         appropriate multiple of pi/2, using extra precision where
         x is close to an exact multiple of pi/2 */
      static const double
        twobypi =  6.36619772367581382433e-01, /* 0x3fe45f306dc9c883 */
        piby2_1  =  1.57079632673412561417e+00, /* 0x3ff921fb54400000 */
        piby2_1tail =  6.07710050650619224932e-11, /* 0x3dd0b4611a626331 */
        piby2_2  =  6.07710050630396597660e-11, /* 0x3dd0b4611a600000 */
        piby2_2tail =  2.02226624879595063154e-21, /* 0x3ba3198a2e037073 */
        piby2_3  =  2.02226624871116645580e-21, /* 0x3ba3198a2e000000 */
        piby2_3tail =  8.47842766036889956997e-32; /* 0x397b839a252049c1 */
      double t, rhead, rtail;
      int npi2;
      unsigned long long uy, xexp, expdiff;
      xexp  = ax >> EXPSHIFTBITS_DP64;
      /* How many pi/2 is x a multiple of? */
      if (ax <= 0x400f6a7a2955385e) /* 5pi/4 */
        {
          if (ax <= 0x4002d97c7f3321d2) /* 3pi/4 */
            npi2 = 1;
          else
            npi2 = 2;
        }
      else if (ax <= 0x401c463abeccb2bb) /* 9pi/4 */
        {
          if (ax <= 0x4015fdbbe9bba775) /* 7pi/4 */
            npi2 = 3;
          else
            npi2 = 4;
        }
      else
        npi2  = (int)(x * twobypi + 0.5);
      /* Subtract the multiple from x to get an extra-precision remainder */
      rhead  = x - npi2 * piby2_1;
      rtail  = npi2 * piby2_1tail;
      GET_BITS_DP64(rhead, uy);
      expdiff = xexp - ((uy & EXPBITS_DP64) >> EXPSHIFTBITS_DP64);
      if (expdiff > 15)
        {
          /* The remainder is pretty small compared with x, which
             implies that x is a near multiple of pi/2
             (x matches the multiple to at least 15 bits) */
          t  = rhead;
          rtail  = npi2 * piby2_2;
          rhead  = t - rtail;
          rtail  = npi2 * piby2_2tail - ((t - rhead) - rtail);
          if (expdiff > 48)
            {
              /* x matches a pi/2 multiple to at least 48 bits */
              t  = rhead;
              rtail  = npi2 * piby2_3;
              rhead  = t - rtail;
              rtail  = npi2 * piby2_3tail - ((t - rhead) - rtail);
            }
        }
      r = rhead - rtail;
      rr = (rhead - r) - rtail;
      region = npi2 & 3;
    }
  else
    {
      /* Reduce x into range [-pi/4,pi/4] */
      __remainder_piby2(x, &r, &rr, &region);
    }

  if (xneg)
    return -tan_piby4(r, rr, region & 1);
  else
    return tan_piby4(r, rr, region & 1);
}
