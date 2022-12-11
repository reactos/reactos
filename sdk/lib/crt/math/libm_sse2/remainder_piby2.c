
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
   extra precision, and return the result in r, rr.
   Return value "region" tells how many lots of pi/2 were subtracted
   from x to put it in the range [-pi/4,pi/4], mod 4. */
void __remainder_piby2(double x, double *r, double *rr, int *region)
{
      /* This method simulates multi-precision floating-point
         arithmetic and is accurate for all 1 <= x < infinity */
      static const double
        piby2_lead = 1.57079632679489655800e+00, /* 0x3ff921fb54442d18 */
        piby2_part1 = 1.57079631090164184570e+00, /* 0x3ff921fb50000000 */
        piby2_part2 = 1.58932547122958567343e-08, /* 0x3e5110b460000000 */
        piby2_part3 = 6.12323399573676480327e-17; /* 0x3c91a62633145c06 */
      const int bitsper = 10;
      unsigned long long res[500];
      unsigned long long ux, u, carry, mask, mant, highbitsrr;
      int first, last, i, rexp, xexp, resexp, ltb, determ;
      double xx, t;
      static unsigned long long pibits[] =
      {
        0,    0,    0,    0,    0,    0,
        162,  998,   54,  915,  580,   84,  671,  777,  855,  839,
        851,  311,  448,  877,  553,  358,  316,  270,  260,  127,
        593,  398,  701,  942,  965,  390,  882,  283,  570,  265,
        221,  184,    6,  292,  750,  642,  465,  584,  463,  903,
        491,  114,  786,  617,  830,  930,   35,  381,  302,  749,
        72,  314,  412,  448,  619,  279,  894,  260,  921,  117,
        569,  525,  307,  637,  156,  529,  504,  751,  505,  160,
        945, 1022,  151, 1023,  480,  358,   15,  956,  753,   98,
        858,   41,  721,  987,  310,  507,  242,  498,  777,  733,
        244,  399,  870,  633,  510,  651,  373,  158,  940,  506,
        997,  965,  947,  833,  825,  990,  165,  164,  746,  431,
        949, 1004,  287,  565,  464,  533,  515,  193,  111,  798
      };

      GET_BITS_DP64(x, ux);


      xexp = (int)(((ux & EXPBITS_DP64) >> EXPSHIFTBITS_DP64) - EXPBIAS_DP64);
      ux = (ux & MANTBITS_DP64) | IMPBIT_DP64;

      /* Now ux is the mantissa bit pattern of x as a long integer */
      carry = 0;
      mask = 1;
      mask = (mask << bitsper) - 1;

      /* Set first and last to the positions of the first
         and last chunks of 2/pi that we need */
      first = xexp / bitsper;
      resexp = xexp - first * bitsper;
      /* 180 is the theoretical maximum number of bits (actually
         175 for IEEE double precision) that we need to extract
         from the middle of 2/pi to compute the reduced argument
         accurately enough for our purposes */
      last = first + 180 / bitsper;

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
         that bitsper is fixed as 10. */
      res[19] = 0;
      u = pibits[last] * ux;
      res[18] = u & mask;
      carry = u >> bitsper;
      u = pibits[last-1] * ux + carry;
      res[17] = u & mask;
      carry = u >> bitsper;
      u = pibits[last-2] * ux + carry;
      res[16] = u & mask;
      carry = u >> bitsper;
      u = pibits[last-3] * ux + carry;
      res[15] = u & mask;
      carry = u >> bitsper;
      u = pibits[last-4] * ux + carry;
      res[14] = u & mask;
      carry = u >> bitsper;
      u = pibits[last-5] * ux + carry;
      res[13] = u & mask;
      carry = u >> bitsper;
      u = pibits[last-6] * ux + carry;
      res[12] = u & mask;
      carry = u >> bitsper;
      u = pibits[last-7] * ux + carry;
      res[11] = u & mask;
      carry = u >> bitsper;
      u = pibits[last-8] * ux + carry;
      res[10] = u & mask;
      carry = u >> bitsper;
      u = pibits[last-9] * ux + carry;
      res[9] = u & mask;
      carry = u >> bitsper;
      u = pibits[last-10] * ux + carry;
      res[8] = u & mask;
      carry = u >> bitsper;
      u = pibits[last-11] * ux + carry;
      res[7] = u & mask;
      carry = u >> bitsper;
      u = pibits[last-12] * ux + carry;
      res[6] = u & mask;
      carry = u >> bitsper;
      u = pibits[last-13] * ux + carry;
      res[5] = u & mask;
      carry = u >> bitsper;
      u = pibits[last-14] * ux + carry;
      res[4] = u & mask;
      carry = u >> bitsper;
      u = pibits[last-15] * ux + carry;
      res[3] = u & mask;
      carry = u >> bitsper;
      u = pibits[last-16] * ux + carry;
      res[2] = u & mask;
      carry = u >> bitsper;
      u = pibits[last-17] * ux + carry;
      res[1] = u & mask;
      carry = u >> bitsper;
      u = pibits[last-18] * ux + carry;
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
          while (mant < 0x0020000000000000)
            {
              i++;
              mant = (mant << bitsper) | (~(res[i]) & mask);
            }
          highbitsrr = ~(res[i + 1]) << (64 - bitsper);
        }
      else
        {
          *region = (ltb >> 1);
          mant = 1;
          mant = res[1] & ((mant << (bitsper - resexp)) - 1);
          while (mant < 0x0020000000000000)
            {
              i++;
              mant = (mant << bitsper) | res[i];
            }
          highbitsrr = res[i + 1] << (64 - bitsper);
        }

      rexp = 52 + resexp - i * bitsper;

      while (mant >= 0x0020000000000000)
        {
          rexp++;
          highbitsrr = (highbitsrr >> 1) | ((mant & 1) << 63);
          mant >>= 1;
        }


      /* Put the result exponent rexp onto the mantissa pattern */
      u = ((unsigned long long)rexp + EXPBIAS_DP64) << EXPSHIFTBITS_DP64;
      ux = (mant & MANTBITS_DP64) | u;
      if (determ)
        /* If we negated the mantissa we negate x too */
        ux |= SIGNBIT_DP64;
      PUT_BITS_DP64(ux, x);

      /* Create the bit pattern for rr */
      highbitsrr >>= 12; /* Note this is shifted one place too far */
      u = ((unsigned long long)rexp + EXPBIAS_DP64 - 53) << EXPSHIFTBITS_DP64;
      PUT_BITS_DP64(u, t);
      u |= highbitsrr;
      PUT_BITS_DP64(u, xx);

      /* Subtract the implicit bit we accidentally added */
      xx -= t;
      /* Set the correct sign, and double to account for the
         "one place too far" shift */
      if (determ)
        xx *= -2.0;
      else
        xx *= 2.0;


      /* (x,xx) is an extra-precise version of the fractional part of
         x * 2 / pi. Multiply (x,xx) by pi/2 in extra precision
         to get the reduced argument (r,rr). */
      {
        double hx, tx, c, cc;
        /* Split x into hx (head) and tx (tail) */
        GET_BITS_DP64(x, ux);
        ux &= 0xfffffffff8000000;
        PUT_BITS_DP64(ux, hx);
        tx = x - hx;

        c = piby2_lead * x;
        cc = ((((piby2_part1 * hx - c) + piby2_part1 * tx) +
               piby2_part2 * hx) + piby2_part2 * tx) +
          (piby2_lead * xx + piby2_part3 * x);
        *r = c + cc;
        *rr = (c - *r) + cc;
      }

  return;
}
