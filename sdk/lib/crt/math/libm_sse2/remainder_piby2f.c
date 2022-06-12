
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


/* Given positive argument x, reduce it to the range [-pi/4,pi/4] using
   extra precision, and return the result in r.
   Return value "region" tells how many lots of pi/2 were subtracted
   from x to put it in the range [-pi/4,pi/4], mod 4. */
void __remainder_piby2f(unsigned long long ux, double *r, int *region)
{


      /* This method simulates multi-precision floating-point
         arithmetic and is accurate for all 1 <= x < infinity */
#define bitsper 36
      unsigned long long res[10];
      unsigned long long u, carry, mask, mant, nextbits;
      int first, last, i, rexp, xexp, resexp, ltb, determ, bc;
      double dx;
      static const double
        piby2 = 1.57079632679489655800e+00; /* 0x3ff921fb54442d18 */
      static unsigned long long pibits[] =
      {
        0LL,
        5215LL, 13000023176LL, 11362338026LL, 67174558139LL,
        34819822259LL, 10612056195LL, 67816420731LL, 57840157550LL,
        19558516809LL, 50025467026LL, 25186875954LL, 18152700886LL
      };


      xexp = (int)(((ux & EXPBITS_DP64) >> EXPSHIFTBITS_DP64) - EXPBIAS_DP64);
      ux = ((ux & MANTBITS_DP64) | IMPBIT_DP64) >> 29;


      /* Now ux is the mantissa bit pattern of x as a long integer */
      mask = 1;
      mask = (mask << bitsper) - 1;

      /* Set first and last to the positions of the first
         and last chunks of 2/pi that we need */
      first = xexp / bitsper;
      resexp = xexp - first * bitsper;
      /* 120 is the theoretical maximum number of bits (actually
         115 for IEEE single precision) that we need to extract
         from the middle of 2/pi to compute the reduced argument
         accurately enough for our purposes */
      last = first + 120 / bitsper;


      /* Do a long multiplication of the bits of 2/pi by the
         integer mantissa */
#if 0
      for (i = last; i >= first; i--)
        {
          u = pibits[i] * ux + carry;
          res[i - first] = u & mask;
          carry = u >> bitsper;
        }
      res[last - first + 1] = 0;
#else
      /* Unroll the loop. This is only correct because we know
         that bitsper is fixed as 36. */
      res[4] = 0;
      u = pibits[last] * ux;
      res[3] = u & mask;
      carry = u >> bitsper;
      u = pibits[last - 1] * ux + carry;
      res[2] = u & mask;
      carry = u >> bitsper;
      u = pibits[last - 2] * ux + carry;
      res[1] = u & mask;
      carry = u >> bitsper;
      u = pibits[first] * ux + carry;
      res[0] = u & mask;
#endif


      /* Reconstruct the result */
      ltb = (int)((((res[0] << bitsper) | res[1])
                   >> (bitsper - 1 - resexp)) & 7);

      /* determ says whether the fractional part is >= 0.5 */
      determ = ltb & 1;

      i = 1;
      if (determ)
        {
          /* The mantissa is >= 0.5. We want to subtract it
             from 1.0 by negating all the bits */
          *region = ((ltb >> 1) + 1) & 3;
          mant = 1;
          mant = ~(res[1]) & ((mant << (bitsper - resexp)) - 1);
          while (mant < 0x0000000000010000)
            {
              i++;
              mant = (mant << bitsper) | (~(res[i]) & mask);
            }
          nextbits = (~(res[i+1]) & mask);
        }
      else
        {
          *region = (ltb >> 1);
          mant = 1;
          mant = res[1] & ((mant << (bitsper - resexp)) - 1);
          while (mant < 0x0000000000010000)
            {
              i++;
              mant = (mant << bitsper) | res[i];
            }
          nextbits = res[i+1];
        }


      /* Normalize the mantissa. The shift value 6 here, determined by
         trial and error, seems to give optimal speed. */
      bc = 0;
      while (mant < 0x0000400000000000)
        {
          bc += 6;
          mant <<= 6;
        }
      while (mant < 0x0010000000000000)
        {
          bc++;
          mant <<= 1;
        }
      mant |= nextbits >> (bitsper - bc);

      rexp = 52 + resexp - bc - i * bitsper;


      /* Put the result exponent rexp onto the mantissa pattern */
      u = ((unsigned long long)rexp + EXPBIAS_DP64) << EXPSHIFTBITS_DP64;
      ux = (mant & MANTBITS_DP64) | u;
      if (determ)
        /* If we negated the mantissa we negate x too */
        ux |= SIGNBIT_DP64;
      PUT_BITS_DP64(ux, dx);


      /* x is a double precision version of the fractional part of
         x * 2 / pi. Multiply x by pi/2 in double precision
         to get the reduced argument r. */
      *r = dx * piby2;
  return;

}
